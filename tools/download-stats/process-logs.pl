#!/usr/bin/perl

################################################################################
# script initialization

use strict; # protect us from ourselves
use DBI; # database stuff
use Date::Parse;
use POSIX qw(strftime);
use Socket; # DNS queries
use File::Find; # grabbing the list of log files from the filesystem
#use Fcntl ':flock'; # import LOCK_* constants for locking log files

# XXX Probably should use File::Basename for splitting paths up into paths and filenames.

# Stuff that should really go into a config file.
my $root_dir = "/data/ftp-logs";
my $sites = "aol|gatech|indiana|isc|oregonstate|rediris|scarlet|utah"; # XXX Maybe this should be generated by a database query.
my $verbose = 1;
my $DO_REVERSE_DNS_LOOKUPS = 0;

# Figure out what period of time to process the logs from.  We use
# a file's "modification time" attribute to store the most recent time
# at which logs were processed, and we process logs between that time
# and the present (i.e. from last processed time + 1 to the current time).
#my $timestamp_file = "$root_dir/last-processed";
#if (!-e $timestamp_file) {
#    # Create the timestamp file and give it a timestamp way in the past.
#    my $status = system("touch", "-t197001010000", $timestamp_file);
#    if ($status != 0) { die "Couldn't touch $timestamp_file: $!" }
#}
#my ($read_time, $last_processed_time) = (stat($timestamp_file))[8,9];
#my $start_time = $last_processed_time + 1;
#my $end_time = time;
#utime($read_time, $end_time, $timestamp_file) 
#  or die "Can't update timestamp on $timestamp_file: $!";

#CREATE TABLE entries (id INT PRIMARY KEY, protocol VARCHAR(4), protocol_version VARCHAR(5), client VARCHAR(15), date_time DATETIME, method VARCHAR(4), file_id INT, status CHAR(3), bytes INT, site_id TINYINT, log_id INT);

# Regular expressions that grab data from the log entries; pre-defined
# and pre-compiled here for performance.  The backslash in [^\"]
# isn't necessary for Perl but fixes indenting confusion in emacs.
my $common_log_regex = qr/^(\S+) \S+ \S+ \[([^:]+:\d+:\d+:\d+ [^\]]+)] "(\S+) (.*?) (\S+)\/(\S+)" (\S+) (\S+) "([^\"]*)" "([^\"]*)"/o;
my $aol_log_regex = qr/(\w{3} \w{3} \d\d \d\d:\d\d:\d\d \d{4}) \d+ (\S+) (\d+) (.*?) (\S+) "([^\"]*)" "([^\"]*)"/o;

################################################################################
# database and query configuration

# Establish a database connection.
my $dsn = "DBI:mysql:host=mecha.mozilla.org;database=downloadstats;port=3306";
my $dbh = DBI->connect($dsn,
                       "logprocessord",
                       "1ssw?w?",
                       { RaiseError => 1,
                         PrintError => 0,
                         ShowErrorStatement => 1 }
                      );

# Prepare the statements we're going to use to insert HTTP log entries into
# the database.
my $insert_entry_sth = $dbh->prepare("INSERT INTO entries (id, protocol, protocol_version, 
                                      client, date_time, method, file_id, status, bytes, site_id, log_id)
                                      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
my $insert_file_sth = $dbh->prepare("INSERT INTO files (id, path, name) VALUES (?, ?, ?)");
my $get_file_id_sth = $dbh->prepare("SELECT id FROM files WHERE path = ? AND name = ?");
my $get_log_status_sth = $dbh->prepare("SELECT id, status FROM logs WHERE path = ? AND name = ?");
my $insert_log_sth = $dbh->prepare("INSERT INTO logs (id, path, name, site_id, status) VALUES (?, ?, ?, ?, ?)");
my $update_log_sth = $dbh->prepare("UPDATE logs SET status = ? WHERE id = ?");
my $get_site_id_sth = $dbh->prepare("SELECT id FROM sites WHERE abbr = ?");

# Get the last unique ID from the database so we know what the next one 
# should be. XXX These assume only one script process will ever be running
# at a time, which is an unsafe assumption; fix this by locking the tables
# in question whenever a new entry is to be inserted and then running
# these queries to get the maximum IDs.  Note that locking could be expensive,
# so perhaps it's better just to lock everything at the beginning and not let
# a second process access the database at all.
my ($entry_id) = $dbh->selectrow_array("SELECT MAX(id) FROM entries") || 0;
my ($max_file_id) = $dbh->selectrow_array("SELECT MAX(id) FROM files") || 0;

my $seen = 0;
my $entered = 0;

my ($client, $date_time, $method, $file, $protocol, $protocol_version, 
    $status, $bytes, $referer, $user_agent, $host, $file_id, $path, $filename);

my %hosts;
my %files;


################################################################################
# main body

find(\&process_log, $root_dir);

################################################################################
# functions

sub process_log {
    # Processes a log file, inserting relevant entries into the database.
    # Called from File::Find::find with $_ containing the filename, 
    # $File::Find::dir containing the path, and $File::Find::name 
    # containing the path + name.

    my $log_seen = 0;
    my $log_entered = 0;

    $File::Find::name && $File::Find::dir && $_ 
      or die "process_log() called without name of file: $File::Find::name\n";

    my $logfile = $_;
    $File::Find::dir =~ m|^$root_dir/(.*)$|;
    my $relative_path = $1 || $File::Find::dir;

    # Don't process the file if it's a directory.
    if (-d $logfile) {
	print "Not processing $File::Find::name; directory\n";
	return;
    }

    # Don't process the file if it isn't a log file.
    # XXX This test may be too brittle, assuming a certain directory
    # and file structure.  It does, however, deal with HTTP logs
    # which aren't in an http/ subdirectory of the site directory.
    if ($File::Find::name !~ m|^$root_dir/($sites)/(http/)?$logfile$|) {
	print "Not processing $File::Find::name; not an HTTP log file.\n";
	return;
    }

    # Grab the site's unique ID from the sites table.
    my $site = $1;
    my ($site_id) = $dbh->selectrow_array($get_site_id_sth, {}, $site);
    if (!$site_id) {
	print "Not processing $File::Find::name; couldn't find an entry " . 
	             "in the sites table for $site.\n";
	return;
    }

    # The name of the log file without any suffix indicating compression.
    # This script treats foo.log and foo.log.gz as the same file so we can
    # compress logs after processing them and they won't get reprocessed.
    # Note that the "name" column in the "logs" table stores the base name
    # of the log file, not the actual name.
    my $basename = $logfile;
    $basename =~ s/\.(gz|bz2)$//;

    # Get the log file's unique ID and status from the database.
    my ($log_id, $status) = 
	$dbh->selectrow_array($get_log_status_sth, {}, $relative_path, $basename);

    if (!$log_id) {
	print "Creating entry in database for log $File::Find::name.\n";
	#$dbh->do("LOCK TABLES logs WRITE");
	($log_id) = $dbh->selectrow_array("SELECT MAX(id) FROM logs");
	$log_id = ($log_id || 0) + 1;
	$insert_log_sth->execute($log_id, $relative_path, $basename, $site_id, "new");
	#$dbh->do("UNLOCK TABLES");
    }
    elsif ($status eq "processed") {
	print "Not processing log $File::Find::name; already processed.\n";
	return;
    }
    elsif ($status eq "processing") {
	print "Not processing log $File::Find::name; already being processed.\n";
	return;
    }

    print "Processing $File::Find::name.\n";

    $update_log_sth->execute("processing", $log_id) || die $dbh->errstr;

    if ($logfile =~ /\.gz$/) {
	open(LOGFILE, "gunzip -c $File::Find::name |")
	  or die "Couldn't open gzipped file $File::Find::name for reading: $!";
    }
    elsif ($logfile =~ /\.bz2$/) {
	open(LOGFILE, "bunzip2 -c $File::Find::name |")
	  or die "Couldn't open bzip2ed file $File::Find::name for reading: $!";
    }
    else {
	open(LOGFILE, "< $File::Find::name")
	  or die "Couldn't open file for reading: $!";
    }
    while (<LOGFILE>) {
        # Periodically print out a message about our progress
        # so users know if the script has frozen or is going slowly.
        ++$seen;
        ++$log_seen;
        print "Processed $log_entered/$log_seen entries for $relative_path/$logfile ($entered/$seen total).\n"
	    if ($seen % 1000 == 0) && $verbose;

	if ($File::Find::name =~ /\.http_trans$/) {
	    ($date_time, $client, $bytes, $file, $status, $user_agent, $referer)
		= ($_ =~ $aol_log_regex);
	    $method = $protocol = $protocol_version = undef;
	}
	else {
	    ($client, $date_time, $method, $file, $protocol, $protocol_version, 
             $status, $bytes, $referer, $user_agent) = ($_ =~ $common_log_regex);
	}
	#print "$client, $date_time, $method, $file, $protocol, $protocol_version, $status, $bytes, $referer, $user_agent\n";

        # Count only successful requests (whether partial or full).
        next unless $status == 200 || $status == 206;

        # Split up the file string into a path and a name.
        $file =~ /^(.*)\/([^\/]*)$/;
        ($path, $filename) = ($1, $2);

        # Only deal with releases, webtools, and language packs at this point.
	next if $path !~ /releases/ && $path !~ /webtools/ && $path !~ /mozilla\/l10n\/lang/;

        # Strip the URL query string, if any, from the filename.
        $filename = (split(/\?/, $filename))[0];

        # Don't bother storing directory accesses, since we don't do anything with them.
	next if !$filename;

        # Get the file's unique ID or create a record for it if none exists yet.
        $file_id = $files{$file};
        if (!$file_id) {
            ($file_id) = $dbh->selectrow_array($get_file_id_sth, {}, $path, $filename);
	    if ($file_id) { $files{$file} = $file_id }
            else {
	        $file_id = ++$max_file_id;
	        $insert_file_sth->execute($file_id, $path, $filename || undef);
            }
        }

        # Convert the timestamp into MySQL's format (including folding the timezone
        # into the time to convert it to local time, since MySQL DATETIME types
        # don't store timezone information).
        $date_time = strftime("%Y/%m/%d %H:%M:%S", localtime(str2time($date_time)));

        if ($DO_REVERSE_DNS_LOOKUPS) {
	    # Do a reverse DNS lookup to get the domain name from the IP address.
	    $host = $hosts{$client};
            if (!$host) {
		if ($client =~ /\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/) {
		    $host = gethostbyaddr(inet_aton($client), AF_INET) || $client;
		}
		else {
		    $host = $client;
		}
		$hosts{$client} = $host;
            }
        }
	else {
	    $host = $client;
	}
	#print "$client = $host\n";

        # Insert the log entry into the database.  We increment
        # the ID so this entry has the next unique ID, and we make
        # the filename be NULL if it doesn't exist because that's
        # easier for queries to understand than a blank string.
        ++$entry_id;
        ++$entered;
        ++$log_entered;
        $insert_entry_sth->execute($entry_id, $protocol, $protocol_version, $host, 
                                   $date_time, $method, $file_id, $status, $bytes, 
				   $site_id, $log_id);
    }
    close(LOGFILE);
    $update_log_sth->execute("processed", $log_id) || die $dbh->errstr;
}
