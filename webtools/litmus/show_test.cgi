#!/usr/bin/perl -w
# -*- Mode: cperl; indent-tabs-mode: nil -*-
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
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Zach Lipton <zach@zachlipton.com>

use strict;

use Litmus;
use Litmus::Error;
use Litmus::DB::Product;
use Litmus::FormWidget;
use Litmus::UserAgentDetect;
use Litmus::SysConfig;
use Litmus::Auth;
use Litmus::Utils;
use Litmus::DB::Resultbug;

use CGI;
use Date::Manip;
use JSON -convert_blessed_universally;

Litmus->init();
my $c = Litmus->cgi(); 

print $c->header();

my $vars;
my $cookie =  Litmus::Auth::getCookie();
my $show_admin = Litmus::Auth::istrusted($cookie);
$vars->{"defaultemail"} = $cookie;
$vars->{"show_admin"} = $show_admin;

$vars->{'default_match_limit'} = Litmus::DB::Testcase->getDefaultMatchLimit();
$vars->{'default_num_days'} = Litmus::DB::Testcase->getDefaultNumDays();
$vars->{'default_relevance_threshold'} = Litmus::DB::Testcase->getDefaultRelevanceThreshold();

my $products = Litmus::FormWidget->getProducts($show_admin);
my $branches = Litmus::FormWidget->getBranches($show_admin);
my $testgroups = Litmus::FormWidget->getTestgroups($show_admin);
my $subgroups = Litmus::FormWidget->getSubgroups($show_admin,'sort_order');
my $platforms = Litmus::FormWidget->getUniquePlatforms($show_admin);
my $opsyses = Litmus::FormWidget->getOpsyses($show_admin);
my $locales = Litmus::FormWidget->getLocales();

my $json = new JSON;
my $products_js = $json->allow_unknown->convert_blessed->encode($products);
my $branches_js = $json->allow_unknown->convert_blessed->encode($branches);
my $testgroups_js = $json->allow_unknown->convert_blessed->encode($testgroups);
my $subgroups_js = $json->allow_unknown->convert_blessed->encode($subgroups);
my $platforms_js = $json->allow_unknown->convert_blessed->encode($platforms);
my $opsyses_js = $json->allow_unknown->convert_blessed->encode($opsyses);

$vars->{'products_js'} = $products_js;
$vars->{'branches_js'} = $branches_js;
$vars->{'testgroups_js'} = $testgroups_js;
$vars->{'subgroups_js'} = $subgroups_js;
$vars->{'platforms_js'} = $platforms_js;
$vars->{'opsyses_js'} = $opsyses_js;
$vars->{'locales'} = $locales;

if (! $c->param) {
    Litmus->template()->process("show/search_for_testcases.tmpl", $vars) || 
        internalError(Litmus->template()->error());
    exit;
} 

if ($c->param("id")) {
  my $testcase_id = $c->param("id");
  my $page = $c->param("page") || 1;

  # Process changes to testcases:
  # Only users with canedit can edit testcases.
  my @changed;
  my $update_status = 0;
  if ($c->param("editingTestcases") && 
      Litmus::Auth::canEdit(Litmus::Auth::getCookie())) {
    # the editingTestcases param contains a comma-separated list of 
    # testcase IDs that the user has made changes to (well, has clicked 
    # the edit button for). 
    @changed = split(',' => $c->param("editingTestcases"));
    foreach my $editid (@changed) {
      my $edittest = Litmus::DB::Testcase->retrieve($editid);
      if (! $edittest) {invalidInputError("Testcase $editid does not exist")}

      $edittest->summary($c->param("summary_edit_$editid"));
      my $product = Litmus::DB::Product->retrieve($c->param("product_$editid"));
      requireField("product", $product);
      $edittest->product($product);
      $edittest->steps($c->param("steps_edit_$editid"));
      $edittest->expected_results($c->param("results_edit_$editid"));

      if ($c->param("enabled_$editid")) {
        $edittest->enabled(1);
        if ($c->param("communityenabled_$editid")) {
          $edittest->communityenabled(1);
        } else {
          $edittest->communityenabled(0);
        }
      } else {
        $edittest->enabled(0);
        $edittest->communityenabled(0);
      }
      my $r_bug_id = $c->param("regression_bug_id_$editid");
      if ($r_bug_id eq '' or $r_bug_id eq '0') {
        undef $r_bug_id;
      }
      $edittest->regression_bug_id($r_bug_id);
      #      $edittest->sort_order($c->param("sort_order_$editid"));
      $edittest->author_id(Litmus::Auth::getCurrentUser());
      $edittest->last_updated(&Date::Manip::UnixDate("now","%q"));
      $edittest->version($edittest->version()+1);

      $update_status = $edittest->update();

      my $status = "";
      my $message = "";
      if ($update_status) {
        $status = "success";
        $message = "Testcase ID# $editid updated successfully.";
      } else {
        $status = "failure";
        $message = "Unable to update testcase ID# $editid.";
      }
      $vars->{'onload'} = "toggleMessage('$status','$message');";
    }
  } elsif ($c->param("editingTestcases") && 
           ! Litmus::Auth::canEdit(Litmus::Auth::getCookie())) {
    invalidInputError("You do not have permissions to edit testcases. ");
  }

  my $testcase = Litmus::DB::Testcase->retrieve($testcase_id);

  if (! $testcase) {
    invalidInputError("\"$testcase_id\" is not a valid testcase ID.");
  }

  my @testgroups = Litmus::DB::Testgroup->search_UniqueEnabledByTestcase($testcase_id);
  my @subgroups = Litmus::DB::Subgroup->search_UniqueEnabledByTestcase($testcase_id);

  my @result_statuses = Litmus::DB::ResultStatus->retrieve_all();

  my $showallresults = $c->param("showallresults") || "";

  my @where;
  push @where, { field => 'testcase', value => $testcase_id };
  my @order_by;
  push @order_by, { field => 'created', direction => 'DESC' };
  my ($test_results,$pager) = Litmus::DB::Testresult->getTestResults(\@where,
                                                                     \@order_by,
                                                                     undef,
                                                                     $page);

  my ($status, $message);
  if ($c->param('resultSubmission')) {
    if ($c->param('resultSubmission')) {
      $status = 'success';
      $message = 'Test result submitted successfully. See the list of test results below.';
    }
  }

  $vars->{'testcase'} = $testcase;
  $vars->{'sysconfig'} = Litmus::SysConfig->getCookie($testcase->product());
  $vars->{'testgroups'} = \@testgroups;
  $vars->{'subgroups'} = \@subgroups;
  $vars->{'result_statuses'} = \@result_statuses;
  $vars->{'showallresults'} = $showallresults;
  $vars->{'test_results'} = $test_results;
  $vars->{'pager'} = $pager;

  if ($status and $message) {
    $vars->{'onload'} = "toggleMessage('$status','$message');";
  }

  Litmus->template()->process("show/show.html.tmpl", $vars) || 
    internalError(Litmus->template()->error());    

  exit;
}

if ($c->param('searchType')) {
  if ($c->param('searchType') eq 'fulltext') {
    if ($c->param("text_snippet")) {
      my $text_snippet = $c->param("text_snippet");
      # Upgrade to utf8 prior to search. 
      utf8::upgrade($text_snippet);
      my $match_limit = $c->param("match_limit");
      my $relevance_threshold = $c->param("relevance_threshold");
      my @testcases = Litmus::DB::Testcase->getFullTextMatches($text_snippet,
                                                               $match_limit,
                                                               $relevance_threshold);
      $vars->{'testcases'} = \@testcases;
      # Decode text snippet again before display.
      utf8::decode($text_snippet);
      $vars->{'search_string_for_display'} = "Full-Text Search: \"$text_snippet\"";
      $vars->{'fulltext'} = 1;
    }
  } elsif ($c->param('searchType') eq 'recent') {
    if ($c->param("recently")) {
      my $recently = $c->param("recently");
      my $match_limit = $c->param("match_limit");
      my $num_days = $c->param("num_days") || Litmus::DB::Testcase->getDefaultNumDays();
      my @testcases;
      my $search_string_for_display;
      if ($recently eq 'added') {
        @testcases = Litmus::DB::Testcase->getNewTestcases(
                                                           $num_days,
                                                           $match_limit
                                                          );
        $search_string_for_display = "Testcases added in the last $num_days days";
      } elsif ($recently eq 'changed') {
        @testcases = Litmus::DB::Testcase->getRecentlyUpdated(
                                                              $num_days,
                                                              $match_limit
                                                             );
        $search_string_for_display = "Testcases changed in the last $num_days days";
      }
      $vars->{'testcases'} = \@testcases;
      $vars->{'search_string_for_display'} = $search_string_for_display;
    }
  } elsif ($c->param('searchType') eq 'by_category') {
    my $product_id = $c->param("product_id");
    my $testgroup_id = $c->param("testgroup_id");
    my $subgroup_id = $c->param("subgroup_id");
    my ($product, $testgroup, $subgroup);
    my @testcases;

    if ($subgroup_id && $subgroup_id ne '-Subgroup-' && $subgroup_id ne '---') {
      @testcases = Litmus::DB::Testcase->search_BySubgroup($subgroup_id);
      $subgroup = Litmus::DB::Subgroup->retrieve($subgroup_id);
      $testgroup = Litmus::DB::Testgroup->retrieve($testgroup_id);
      $product = Litmus::DB::Product->retrieve($product_id);
    } elsif ($testgroup_id && $testgroup_id ne '-Testgroup-' && $testgroup_id ne '---') {
      @testcases = Litmus::DB::Testcase->search_ByTestgroup($testgroup_id);
      $testgroup = Litmus::DB::Testgroup->retrieve($testgroup_id);
      $product = Litmus::DB::Product->retrieve($product_id);
    } elsif ($product_id && $product_id ne '-Product-' && $product_id ne '---') {
      @testcases = Litmus::DB::Testcase->search(product => $product_id);
      $product = Litmus::DB::Product->retrieve($product_id);
    }
    $vars->{'testcases'} = \@testcases;
    $vars->{'search_string_for_display'} = 
      ($product ? "product: ".$product->name() : '').
        ($testgroup ? " | testgroup: ".$testgroup->name() : '').
    	  ($subgroup ? " | subgroup: ".$subgroup->name() : ''); 

  } elsif ($c->param('searchType') eq 'ungrouped') {
      my @testcases = Litmus::DB::Testcase->search_Ungrouped();
      my $search_string_for_display = "Testcases not associated with any subgroup.";
      $vars->{'testcases'} = \@testcases;
      $vars->{'search_string_for_display'} = $search_string_for_display;
  }
  $vars->{'searchType'} = $c->param('searchType');
}

if ($c->param('print')) {
  Litmus->template()->process("show/print_testcases.tmpl", $vars) || 
    internalError(Litmus->template()->error());
} else {
  Litmus->template()->process("show/search_for_testcases.tmpl", $vars) || 
    internalError(Litmus->template()->error());
}
