#!/usr/bonsaitools/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-

#
# This is a script to edit the target milestones. It is largely a copy of
# the editversions.cgi script, since the two fields were set up in a
# very similar fashion.
#
# (basically replace each occurance of 'milestone' with 'version', and
# you'll have the original script)
#
# Matt Masson <matthew@zeroknowledge.com>
#


use diagnostics;
use strict;

require "CGI.pl";
require "globals.pl";




# TestProduct:  just returns if the specified product does exists
# CheckProduct: same check, optionally  emit an error text
# TestMilestone:  just returns if the specified product/version combination exists
# CheckMilestone: same check, optionally emit an error text

sub TestProduct ($)
{
    my $prod = shift;

    # does the product exist?
    SendSQL("SELECT product
             FROM products
             WHERE product=" . SqlQuote($prod));
    return FetchOneColumn();
}

sub CheckProduct ($)
{
    my $prod = shift;

    # do we have a product?
    unless ($prod) {
        print "Sorry, you haven't specified a product.";
        PutTrailer();
        exit;
    }

    unless (TestProduct $prod) {
        print "Sorry, product '$prod' does not exist.";
        PutTrailer();
        exit;
    }
}

sub TestMilestone ($$)
{
    my ($prod,$mile) = @_;

    # does the product exist?
    SendSQL("SELECT product,value
             FROM milestones
             WHERE product=" . SqlQuote($prod) . " and value=" . SqlQuote($mile));
    return FetchOneColumn();
}

sub CheckMilestone ($$)
{
    my ($prod,$mile) = @_;

    # do we have the milestone?
    unless ($mile) {
        print "Sorry, you haven't specified a milestone.";
        PutTrailer();
        exit;
    }

    CheckProduct($prod);

    unless (TestMilestone $prod,$mile) {
        print "Sorry, milestone '$mile' for product '$prod' does not exist.";
        PutTrailer();
        exit;
    }
}


#
# Displays the form to edit a milestone
#

sub EmitFormElements ($$$)
{
    my ($product, $milestone, $sortkey) = @_;

    print "  <TH ALIGN=\"right\">Milestone:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=64 NAME=\"milestone\" VALUE=\"" .
        value_quote($milestone) . "\">\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"right\">Sortkey:</TH>\n";
    print "  <TD><INPUT SIZE=64 MAXLENGTH=64 NAME=\"sortkey\" VALUE=\"" .
        value_quote($sortkey) . "\">\n";
    print "      <INPUT TYPE=HIDDEN NAME=\"product\" VALUE=\"" .
        value_quote($product) . "\"></TD>\n";
}


#
# Displays a text like "a.", "a or b.", "a, b or c.", "a, b, c or d."
#

sub PutTrailer (@)
{
    my (@links) = ("Back to the <A HREF=\"query.cgi\">query page</A>", @_);

    my $count = $#links;
    my $num = 0;
    print "<P>\n";
    foreach (@links) {
        print $_;
        if ($num == $count) {
            print ".\n";
        }
        elsif ($num == $count-1) {
            print " or ";
        }
        else {
            print ", ";
        }
        $num++;
    }
    PutFooter();
}







#
# Preliminary checks:
#

confirm_login();

print "Content-type: text/html\n\n";

unless (UserInGroup("editcomponents")) {
    PutHeader("Not allowed");
    print "Sorry, you aren't a member of the 'editcomponents' group.\n";
    print "And so, you aren't allowed to add, modify or delete milestones.\n";
    PutTrailer();
    exit;
}


#
# often used variables
#
my $product = trim($::FORM{product} || '');
my $milestone = trim($::FORM{milestone} || '');
my $sortkey = trim($::FORM{sortkey} || '0');
my $action  = trim($::FORM{action}  || '');
my $localtrailer;
if ($milestone) {
    $localtrailer = "<A HREF=\"editmilestones.cgi?product=" . url_quote($product) . "\">edit</A> more milestones";
} else {
    $localtrailer = "<A HREF=\"editmilestones.cgi\">edit</A> more milestones";
}



#
# product = '' -> Show nice list of milestones
#

unless ($product) {
    PutHeader("Select product");

    SendSQL("SELECT products.product,products.description,'xyzzy'
             FROM products 
             GROUP BY products.product
             ORDER BY products.product");
    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH ALIGN=\"left\">Edit milestones of ...</TH>\n";
    print "  <TH ALIGN=\"left\">Description</TH>\n";
    print "  <TH ALIGN=\"left\">Bugs</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($product, $description, $bugs) = FetchSQLData();
        $description ||= "<FONT COLOR=\"red\">missing</FONT>";
        $bugs ||= "none";
        print "<TR>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editmilestones.cgi?product=", url_quote($product), "\"><B>$product</B></A></TD>\n";
        print "  <TD VALIGN=\"top\">$description</TD>\n";
        print "  <TD VALIGN=\"top\">$bugs</TD>\n";
    }
    print "</TR></TABLE>\n";

    PutTrailer();
    exit;
}



#
# action='' -> Show nice list of milestones
#

unless ($action) {
    PutHeader("Select milestone for $product");
    CheckProduct($product);

    SendSQL("SELECT value,sortkey
             FROM milestones
             WHERE product=" . SqlQuote($product) . "
             ORDER BY sortkey,value");

    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0><TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH ALIGN=\"left\">Edit milestone ...</TH>\n";
    #print "  <TH ALIGN=\"left\">Bugs</TH>\n";
    print "  <TH ALIGN=\"left\">Sortkey</TH>\n";
    print "  <TH ALIGN=\"left\">Action</TH>\n";
    print "</TR>";
    while ( MoreSQLData() ) {
        my ($milestone,$sortkey,$bugs) = FetchSQLData();
        $bugs ||= 'none';
        print "<TR>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editmilestones.cgi?product=", url_quote($product), "&milestone=", url_quote($milestone), "&action=edit\"><B>$milestone</B></A></TD>\n";
        #print "  <TD VALIGN=\"top\">$bugs</TD>\n";
        print "  <TD VALIGN=\"top\" ALIGN=\"right\">$sortkey</TD>\n";
        print "  <TD VALIGN=\"top\"><A HREF=\"editmilestones.cgi?product=", url_quote($product), "&milestone=", url_quote($milestone), "&action=del\"><B>Delete</B></A></TD>\n";
        print "</TR>";
    }
    print "<TR>\n";
    print "  <TD VALIGN=\"top\" COLSPAN=\"2\">Add a new milestone</TD>\n";
    print "  <TD VALIGN=\"top\" ALIGN=\"middle\"><A HREF=\"editmilestones.cgi?product=", url_quote($product) . "&action=add\">Add</A></TD>\n";
    print "</TR></TABLE>\n";

    PutTrailer();
    exit;
}




#
# action='add' -> present form for parameters for new milestone
#
# (next action will be 'new')
#

if ($action eq 'add') {
    PutHeader("Add milestone for $product");
    CheckProduct($product);

    #print "This page lets you add a new milestone to a $::bugzilla_name tracked product.\n";

    print "<FORM METHOD=POST ACTION=editmilestones.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($product, $milestone, 0);

    print "</TABLE>\n<HR>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Add\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"new\">\n";
    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='new' -> add milestone entered in the 'action=add' screen
#

if ($action eq 'new') {
    PutHeader("Adding new milestone for $product");
    CheckProduct($product);

    # Cleanups and valididy checks

    unless ($milestone) {
        print "You must enter a text for the new milestone. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }
    if (TestMilestone($product,$milestone)) {
        print "The milestone '$milestone' already exists. Please press\n";
        print "<b>Back</b> and try again.\n";
        PutTrailer($localtrailer);
        exit;
    }

    # Add the new milestone
    SendSQL("INSERT INTO milestones ( " .
          "value, product, sortkey" .
          " ) VALUES ( " .
          SqlQuote($milestone) . "," .
          SqlQuote($product) . ", $sortkey)");

    # Make versioncache flush
    unlink "data/versioncache";

    print "OK, done.<p>\n";
    PutTrailer($localtrailer);
    exit;
}




#
# action='del' -> ask if user really wants to delete
#
# (next action would be 'delete')
#

if ($action eq 'del') {
    PutHeader("Delete milestone of $product");
    CheckMilestone($product, $milestone);

    SendSQL("SELECT count(bug_id),product,target_milestone
             FROM bugs
             GROUP BY product,target_milestone
             HAVING product=" . SqlQuote($product) . "
                AND target_milestone=" . SqlQuote($milestone));
    my $bugs = FetchOneColumn();

    SendSQL("SELECT defaultmilestone FROM products " .
            "WHERE product=" . SqlQuote($product));
    my $defaultmilestone = FetchOneColumn();

    print "<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=0>\n";
    print "<TR BGCOLOR=\"#6666FF\">\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Part</TH>\n";
    print "  <TH VALIGN=\"top\" ALIGN=\"left\">Value</TH>\n";

    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Product:</TH>\n";
    print "  <TD VALIGN=\"top\">$product</TD>\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Milestone:</TH>\n";
    print "  <TD VALIGN=\"top\">$milestone</TD>\n";
    print "</TR><TR>\n";
    print "  <TH ALIGN=\"left\" VALIGN=\"top\">Bugs:</TH>\n";
    print "  <TD VALIGN=\"top\">", $bugs || 'none' , "</TD>\n";
    print "</TR></TABLE>\n";

    print "<H2>Confirmation</H2>\n";

    if ($bugs) {
        if (!Param("allowbugdeletion")) {
            print "Sorry, there are $bugs bugs outstanding for this milestone.
You must reassign those bugs to another milestone before you can delete this
one.";
            PutTrailer($localtrailer);
            exit;
        }
        print "<TABLE BORDER=0 CELLPADDING=20 WIDTH=\"70%\" BGCOLOR=\"red\"><TR><TD>\n",
              "There are bugs entered for this milestone!  When you delete this ",
              "milestone, <B><BLINK>all</BLINK></B> stored bugs will be deleted, too. ",
              "You could not even see the bug history for this milestone anymore!\n",
              "</TD></TR></TABLE>\n";
    }

    if ($defaultmilestone eq $milestone) {
        print "Sorry; this is the default milestone for this product, and " .
            "so it can not be deleted.";
        PutTrailer($localtrailer);
        exit;
    }

    print "<P>Do you really want to delete this milestone?<P>\n";
    print "<FORM METHOD=POST ACTION=editmilestones.cgi>\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Yes, delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"delete\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"product\" VALUE=\"" .
        value_quote($product) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"milestone\" VALUE=\"" .
        value_quote($milestone) . "\">\n";
    print "</FORM>";

    PutTrailer($localtrailer);
    exit;
}



#
# action='delete' -> really delete the milestone
#

if ($action eq 'delete') {
    PutHeader("Deleting milestone of $product");
    CheckMilestone($product,$milestone);

    # lock the tables before we start to change everything:

    SendSQL("LOCK TABLES attachments WRITE,
                         bugs WRITE,
                         bugs_activity WRITE,
                         milestones WRITE,
                         dependencies WRITE");

    # According to MySQL doc I cannot do a DELETE x.* FROM x JOIN Y,
    # so I have to iterate over bugs and delete all the indivial entries
    # in bugs_activies and attachments.

    if (Param("allowbugdeletion")) {

        SendSQL("SELECT bug_id
             FROM bugs
             WHERE product=" . SqlQuote($product) . "
               AND target_milestone=" . SqlQuote($milestone));
        while (MoreSQLData()) {
            my $bugid = FetchOneColumn();

            my $query =
                $::db->query("DELETE FROM attachments WHERE bug_id=$bugid")
                or die "$::db_errstr";
            $query =
                $::db->query("DELETE FROM bugs_activity WHERE bug_id=$bugid")
                or die "$::db_errstr";
            $query =
                $::db->query("DELETE FROM dependencies WHERE blocked=$bugid")
                or die "$::db_errstr";
        }
        print "Attachments, bug activity and dependencies deleted.<BR>\n";


        # Deleting the rest is easier:

        SendSQL("DELETE FROM bugs
             WHERE product=" . SqlQuote($product) . "
               AND target_milestone=" . SqlQuote($milestone));
        print "Bugs deleted.<BR>\n";
    }

    SendSQL("DELETE FROM milestones
             WHERE product=" . SqlQuote($product) . "
               AND value=" . SqlQuote($milestone));
    print "Milestone deleted.<P>\n";
    SendSQL("UNLOCK TABLES");

    unlink "data/versioncache";
    PutTrailer($localtrailer);
    exit;
}



#
# action='edit' -> present the edit milestone form
#
# (next action would be 'update')
#

if ($action eq 'edit') {
    PutHeader("Edit milestone of $product");
    CheckMilestone($product,$milestone);

    SendSQL("SELECT sortkey FROM milestones WHERE product=" .
            SqlQuote($product) . " AND value = " . SqlQuote($milestone));
    my $sortkey = FetchOneColumn();

    print "<FORM METHOD=POST ACTION=editmilestones.cgi>\n";
    print "<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0><TR>\n";

    EmitFormElements($product, $milestone, $sortkey);

    print "</TR></TABLE>\n";

    print "<INPUT TYPE=HIDDEN NAME=\"milestoneold\" VALUE=\"" .
        value_quote($milestone) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"sortkeyold\" VALUE=\"" .
        value_quote($sortkey) . "\">\n";
    print "<INPUT TYPE=HIDDEN NAME=\"action\" VALUE=\"update\">\n";
    print "<INPUT TYPE=SUBMIT VALUE=\"Update\">\n";

    print "</FORM>";

    my $other = $localtrailer;
    $other =~ s/more/other/;
    PutTrailer($other);
    exit;
}



#
# action='update' -> update the milestone
#

if ($action eq 'update') {
    PutHeader("Update milestone of $product");

    my $milestoneold = trim($::FORM{milestoneold} || '');
    my $sortkeyold = trim($::FORM{sortkeyold} || '0');

    CheckMilestone($product,$milestoneold);

    SendSQL("LOCK TABLES bugs WRITE,
                         milestones WRITE,
                         products WRITE");

    if ($milestone ne $milestoneold) {
        unless ($milestone) {
            print "Sorry, I can't delete the milestone text.";
            PutTrailer($localtrailer);
	    SendSQL("UNLOCK TABLES");
            exit;
        }
        if (TestMilestone($product,$milestone)) {
            print "Sorry, milestone '$milestone' is already in use.";
            PutTrailer($localtrailer);
	    SendSQL("UNLOCK TABLES");
            exit;
        }
        SendSQL("UPDATE bugs
                 SET target_milestone=" . SqlQuote($milestone) . "
                 WHERE target_milestone=" . SqlQuote($milestoneold) . "
                   AND product=" . SqlQuote($product));
        SendSQL("UPDATE milestones
                 SET value=" . SqlQuote($milestone) . "
                 WHERE product=" . SqlQuote($product) . "
                   AND value=" . SqlQuote($milestoneold));
        SendSQL("UPDATE products " .
                "SET defaultmilestone = " . SqlQuote($milestone) .
                "WHERE product = " . SqlQuote($product) .
                "  AND defaultmilestone = " . SqlQuote($milestoneold));
        unlink "data/versioncache";
        print "Updated milestone.<BR>\n";
    }
    if ($sortkey != $sortkeyold) {
        SendSQL("UPDATE milestones SET sortkey=$sortkey
                 WHERE product=" . SqlQuote($product) . "
                   AND value=" . SqlQuote($milestoneold));
        unlink "data/versioncache";
        print "Updated sortkey.<BR>\n";
    }
    SendSQL("UNLOCK TABLES");

    PutTrailer($localtrailer);
    exit;
}



#
# No valid action found
#

PutHeader("Error");
print "I don't have a clue what you want.<BR>\n";

foreach ( sort keys %::FORM) {
    print "$_: $::FORM{$_}<BR>\n";
}
