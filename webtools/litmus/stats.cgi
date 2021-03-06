#!/usr/bin/perl -w
# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is
# the Mozilla Corporation.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Chris Cooper <ccooper@deadsquid.com>
#   Zach Lipton <zach@zachlipton.com>
#
# ***** END LICENSE BLOCK *****

use strict;

use Litmus;
use Litmus::Auth;
use Litmus::Error;
use Litmus::DB::Product;

use CGI;

Litmus->init();
my $c = Litmus->cgi(); 

print $c->header();

# find the number of testcases in the database
my $numtests = Litmus::DB::Testcase->count_all();

# find the number of subgroups in the database
my $numsubgroups = Litmus::DB::Subgroup->count_all();

# find the number of testgroups in the database
my $numtestgroups = Litmus::DB::Testgroup->count_all();

# find the number of users in the database
my $numusers = Litmus::DB::User->count_all();

# find the number of results in the database:
my $numresults = Litmus::DB::Testresult->count_all();

# get a list of the top 15 testers of all time, sorted by the number 
# of test results submitted:
my @toptesters = Litmus::DB::User->search_TopTesters();

my $vars = {
    title      => "Statistics",
    numtests   => $numtests,
    numsubgroups   => $numsubgroups,
    numtestgroups   => $numtestgroups,
    numusers   => $numusers,
    numresults => $numresults,
    toptesters => \@toptesters,
};

my $cookie =  Litmus::Auth::getCookie();
$vars->{"defaultemail"} = $cookie;
$vars->{"show_admin"} = Litmus::Auth::istrusted($cookie);

Litmus->template()->process("stats/stats.html.tmpl", $vars) || 
        internalError(Litmus->template()->error());    
        
