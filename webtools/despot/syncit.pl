#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is the Despot Account Administration System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>

# $F::debug = 1;


$cvs = "/opt/cvs-tools/bin/cvs";
    
$dontcommit = 0;
$usertoblame = "";

for ($i=0 ; $i<@ARGV ; $i++) {
    if ($ARGV[$i] eq "-n") {
        $dontcommit = 1;
    }
    if ($ARGV[$i] eq "-user") {
        $usertoblame = $ARGV[++$i];
    }
}

$srcdir = $0;
$srcdir =~ s:/[^/]*$::;      # Remove last word, and slash before it.
if ($srcdir eq "") {
    $srcdir = ".";
}
chdir $srcdir || die "Couldn't chdir to $srcdir";

use Mysql;
require 'utils.pl';

$db = Mysql->Connect("localhost", "mozusers")
    || die "Can't connect to database server";

$db = $db;                      # Make -w shut up.

($mylogin = `/usr/ucb/whoami`) || ($mylogin = `/bin/whoami`);
chop($mylogin);

$hostname = 'unknown';
if (open(HOST, "/bin/hostname|")) {
    $hostname = <HOST>;
    chop($hostname);
    close(HOST);
}


$ENV{"HOME"} = glob("~$mylogin"); # CVS wants this.

$curdir = `pwd`;
chop($curdir);
$ENV{"CVS_PASSFILE"} = "$curdir/.cvspass";

if ($usertoblame eq "") {
    $usertoblame = $mylogin;
}

$boilerplate = "";

open(BOILERPLATE, "<commitcheck.templ") || die "Can't open template file";
while (<BOILERPLATE>) {
    if ( /^#/ ) {
        # Strip out comments from the boilerplate.  Might as well; the
        # faster our generated perl script runs, the better.
        next;
    }
    $boilerplate .= $_;
}
close BOILERPLATE;


open(BOILERPLATE, "<owners.templ") || die "Can't open template file";
while (<BOILERPLATE>) {
    push @ownersplate, $_;
}
close BOILERPLATE;




$repquery = Query("select id,name,cvsroot,ownersrepository,ownerspath from repositories order by name");
while (@reprow = $repquery->fetchrow()) {
    ($repid,$repname,$reproot,$ownersrepository,$ownerspath) = (@reprow);

    $query = Query("select email,passwd,${repname}_group,neednewpassword,disabled from users where ${repname}_group != 'None' and passwd != '' order by email");

    $tmpdir = "/tmp/syncit.$$";
    mkdir $tmpdir, 0777;
    chdir $tmpdir;
    
    $ENV{CVSROOT} = $reproot;

    system "$cvs co CVSROOT/passwd CVSROOT/commitcheck.pl" || die "Couldn't checkout files.";
    
    $outfile = "CVSROOT/passwd";
    open(PASSWD, ">$outfile") || die "Can't open $outfile";
    
    print PASSWD "# DO NOT EDIT THIS FILE!  You must instead go to http://warp/mozilla.org, and\n";
    print PASSWD "# tweak things from there.\n";
    
    while (@row = $query->fetchrow()) {
        ($email,$password,$group,$neednew,$disabled) = @row;
        if ($neednew eq "Yes" || $disabled eq "Yes") {
            next;
        }
        $login = $email;
        $login =~ s/@/%/g;
        print PASSWD "$login:$password:$group\n";
    }
    close PASSWD;
    
#    system "$cvs co CVSROOT/commitcheck.pl" || die "Couldn't checkout passwd file.";
    
    $outfile = "CVSROOT/commitcheck.pl";
    open(COMMITCHECK, ">$outfile") || die "Can't open $outfile";

    print COMMITCHECK "#!/tools/ns/bin/perl5.004 --\n";
    print COMMITCHECK "# DO NOT EDIT THIS FILE!  You must instead go to http://cvs-mirror.mozilla.org/webtools/despot, and\n";
    print COMMITCHECK "# tweak things from there.\n\n";

    $query = Query("select partitions.id,partitions.name,state,branches.name from partitions,branches where repositoryid = '$repid' and branches.id=branchid order by partitions.name");
    $founddefault = 0;
    while (@row = $query->fetchrow()) {
        ($id,$name,$state,$branch) = (@row);
        $d = "\$";
        print COMMITCHECK $d . "mode{'$id'} = '$state';\n";
        print COMMITCHECK $d . "branch{'$id'} = '$branch';\n";
        print COMMITCHECK $d . "fullname{'$id'} = '$name';\n";
        if ($name eq 'default') {
            print COMMITCHECK $d . "defaultid = '$id';\n";
            $founddefault = 1;
        }
        if ($state ne "Open") {
            foreach $n ("blessed", "super") {
                print COMMITCHECK $d . "$n" . "{'$id'} = [";
                $eq = "=";
                if ($n eq "super") {
                    $eq = "!=";
                }
                $q2 = Query("select email from members,users where partitionid = $id and class $eq 'Member' and users.id = userid");
                while (@r2 = $q2->fetchrow()) {
                    my $n = $r2[0];
                    $n =~ s/@/%/;
                    print COMMITCHECK "'$n',";
                }
                print COMMITCHECK "];\n";
            }
        }
    }
    if (!$founddefault) {
        print COMMITCHECK $d . "defaultid = 'none';\n";
    }
    print COMMITCHECK "sub GetT {\n";
    print COMMITCHECK '($b,$_) = (@_);' . "\n";
    $query = Query("select branches.name,partitions.id from partitions,branches where repositoryid = '$repid' and branches.id = branchid order by branches.name");
    $lastbranch = "";
    while (@row = $query->fetchrow()) {
        ($branchname,$partid) = (@row);
        if ($branchname ne $lastbranch) {
            if ($lastbranch ne "") {
                print COMMITCHECK "}\n";
            }
            print COMMITCHECK "if (" . $d . "b eq '$branchname') {\n";
            $lastbranch = $branchname;
        }
        $q2 = Query("select pattern from files where partitionid=$partid order by pattern");
        while (@r2 = $q2->fetchrow()) {
            my $regexp = $r2[0];
            $regexp =~ s/\./\\./g;
            $regexp =~ s:\*$:.*:;
            $regexp =~ s:\%$:[^/]*:;
            $regexp = '^' . $regexp . "\$";
            print COMMITCHECK "if (m:$regexp:) {return '$partid';}\n";
        }
    }
    if ($lastbranch ne "") {
        print COMMITCHECK "}\n";
    }
    print COMMITCHECK "return '';\n";
    print COMMITCHECK "}\n";

    print COMMITCHECK $boilerplate;

    close COMMITCHECK;
    
    chdir "CVSROOT";
    if ($dontcommit) {
        system "$cvs diff -c passwd";
        system "$cvs diff -c commitcheck.pl";
        # system "$cvs commit -m 'Pseudo-automatic update of changes made by $usertoblame.' commitcheck.pl";
    } else {
        system "$cvs commit -m 'Pseudo-automatic update of changes made by $usertoblame.'";
    }
    

    if (defined $ownersrepository && $ownersrepository > 0 &&
        defined $ownerspath && $ownerspath ne "") {
        $query = Query("select cvsroot from repositories where id = $ownersrepository");
        $ENV{CVSROOT} = ($query->fetchrow())[0];
        $tdir = "$tmpdir/ownerstuff";
        mkdir $tdir, 0777;
        chdir $tdir;
        system "$cvs co $ownerspath" || die "Couldn't checkout $ownerspath";

        open(OWNERS, ">$ownerspath") || die "Can't open $ownerspath";
        print OWNERS "<!-- THIS FILE IS AUTOMATICALLY GENERATED; DO NOT EDIT. -->\n";

        foreach (@ownersplate) {
            if ($_ !~ m/^%%DATA%%/) {
                print OWNERS $_;
                next;
            }
            $query = Query("select id,name,description,newsgroups,doclinks from partitions where repositoryid = '$repid' order by name");
            while (@row = $query->fetchrow()) {
                ($id,$name,$desc,$newsgroups,$doclinks) = (@row);
                if ($name eq "default" || $name eq "despotaccess") {
                    next;
                }
                my $fullname = $name;
                if (defined $desc && $desc ne "") {
                    $fullname .= " ($desc)";
                }
                $q2 = Query("select class,email,realname from members,users where partitionid = $id and class != 'Member' and users.id = userid");
                my @owners;
                my @ownernames;
                my @peers;
                my @peernames;
                while (@r2 = $q2->fetchrow()) {
                    if ($r2[0] eq "Owner") {
                        push @owners, $r2[1];
                        push @ownernames, $r2[2];
                    } else {
                        push @peers, $r2[1];
                        push @peernames, $r2[2];
                    }
                }

                my $maillist = "mailto:" . join(',', @owners);
                if (@peers > 0) {
                    $maillist .= "?cc=" . join(',', @peers);
                }

                print OWNERS "<A name='$name'>
<TABLE border width='100%' bgcolor='#EEEEEE'>
<TR><TH valign=top align=left colspan=3 bgcolor='#c0c0c0'>
Module: $fullname</TH></TR>
<TR><TD width='5%'></TD><TD width='10%'></TD><TD width='85%'></TD></TR>
<TR><TD></TD><TH valign=top align=left>
Owner:</TH><TD><A href='$maillist'>" .
    join(', ', @ownernames) . "</A></TD></TR>
<TR><TD></TD><TH valign=top align=left>
Source:</TH><TD>";
                $q2 = Query("select pattern from files where partitionid=$id order by pattern");
                my @filelist;
                while (@r2 = $q2->fetchrow()) {
                    my $name = $r2[0];
                    $name =~ s/\*$//;
                    $name =~ s/%$//;
                    $name =~ s:/$::;
                    $name =~ s:^mozilla/::;
                    push @filelist, "<A href='http://cvs-mirror.mozilla.org/webtools/lxr/source/$name'>$name</a>";
                }
                print OWNERS join(', ', @filelist);
                print OWNERS "</TD></TR>
<TR><TD></TD><TH valign=top align=left>
Newsgroup:</TH><TD>";
                my @grouplist;
                if (!defined $newsgroups) {
                    $newsgroups = "";
                }
                foreach $i (split(/,/, $newsgroups)) {
                    my $base = "news:";
                    if ($i =~ /^netscape\.public/) {
                        $base = "news://news.mozilla.org/";
                    }
                    push @grouplist, "<a href='$base$i'>$i</a>";
                }
                print OWNERS join(', ', @grouplist);
                print OWNERS "</TD></TR>
<TR><TD></TD><TH valign=top align=left>
Peers:</TH><TD>";
                my @peerlist;
                foreach $i (@peers) {
                    push @peerlist, "<a href='mailto:$i'>" . pop(@peernames) . "</a>";
                }
                print OWNERS join(', ', @peerlist);
                print OWNERS "</TD></TR>
<TR><TD></TD><TH valign=top align=left>
Documents:</TH><TD>";
                my @doclist;
                if (!defined $doclinks) {
                    $doclinks = "";
                }
                foreach $i (split(/,/, $doclinks)) {
                    push @doclist, "<A href='$i'>$i</a>";
                }
                print OWNERS join(', ', @doclist);
                print OWNERS "</TD></TR>
</TABLE>

";
            }
        }

        close OWNERS;

        system "$cvs commit -m 'Pseudo-automatic update of changes made by $usertoblame.'";
    }
            




    chdir "/";
    
    system "rm -rf $tmpdir";
    
}


Query("delete from syncneeded");
    
