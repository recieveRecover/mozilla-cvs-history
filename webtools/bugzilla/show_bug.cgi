#!/usr/bin/perl -wT
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Bug Tracking System.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Terry Weissman <terry@mozilla.org>

use strict;

use lib qw(.);

use Bugzilla;
use Bugzilla::Constants;

require "CGI.pl";

use vars qw($template $vars $userid);

use Bugzilla::Bug;

my $cgi = Bugzilla->cgi;

if ($cgi->param('GoAheadAndLogIn')) {
    Bugzilla->login(LOGIN_REQUIRED);
} else {
    Bugzilla->login();
}

# Editable, 'single' HTML bugs are treated slightly specially in a few places
my $single = !$cgi->param('format')
  && (!$cgi->param('ctype') || $cgi->param('ctype') eq 'html');

# If we don't have an ID, _AND_ we're only doing a single bug, then prompt
if (!$cgi->param('id') && $single) {
    print Bugzilla->cgi->header();
    $template->process("bug/choose.html.tmpl", $vars) ||
      ThrowTemplateError($template->error());
    exit;
}

my $format = GetFormat("bug/show", scalar $cgi->param('format'), 
                       scalar $cgi->param('ctype'));

GetVersionTable();

my @bugs = ();

if ($single) {
    my $id = $cgi->param('id');
    # Its a bit silly to do the validation twice - that functionality should
    # probably move into Bug.pm at some point
    ValidateBugID($id);
    push @bugs, new Bugzilla::Bug($id, $userid);
} else {
    foreach my $id ($cgi->param('id')) {
        my $bug = new Bugzilla::Bug($id, $userid);
        push @bugs, $bug;
    }
}

# Determine if Patch Viewer is installed, for Diff link
eval {
  require PatchReader;
  $vars->{'patchviewerinstalled'} = 1;
};

$vars->{'bugs'} = \@bugs;

# Next bug in list (if there is one)
my @bug_list;
if ($cgi->cookie("BUGLIST")) {
    @bug_list = split(/:/, $cgi->cookie("BUGLIST"));
}

$vars->{'bug_list'} = \@bug_list;

# Work out which fields we are displaying (currently XML only.)
# If no explicit list is defined, we show all fields. We then exclude any
# on the exclusion list. This is so you can say e.g. "Everything except 
# attachments" without listing almost all the fields.
my @fieldlist = (Bugzilla::Bug::fields(), 'group', 'long_desc', 'attachment');
my %displayfields;

if ($cgi->param("field")) {
    @fieldlist = $cgi->param("field");
}

foreach (@fieldlist) {
    $displayfields{$_} = 1;
}

foreach ($cgi->param("excludefield")) {
    $displayfields{$_} = undef;    
}

$vars->{'displayfields'} = \%displayfields;

print $cgi->header($format->{'ctype'});

$template->process("$format->{'template'}", $vars)
  || ThrowTemplateError($template->error());
