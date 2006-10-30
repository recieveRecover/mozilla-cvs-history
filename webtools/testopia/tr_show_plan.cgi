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
# The Original Code is the Bugzilla Test Runner System.
#
# The Initial Developer of the Original Code is Maciej Maczynski.
# Portions created by Maciej Maczynski are Copyright (C) 2001
# Maciej Maczynski. All Rights Reserved.
#
# Contributor(s): Maciej Maczynski <macmac@xdsnet.pl>
#                 Ed Fuentetaja <efuentetaja@acm.org>
#                 Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::Table;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::TestTag;
use Bugzilla::Testopia::Category;
use Bugzilla::Testopia::Attachment;
use Bugzilla::Testopia::Search;
use Bugzilla::Testopia::Table;
use JSON;

require "globals.pl";

use vars qw($template $vars);
my $template = Bugzilla->template;
my $run_query_limit = 5000;
my $case_query_limit = 10000;

Bugzilla->login();

my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;

my $plan_id = trim(Bugzilla->cgi->param('plan_id') || '');

unless ($plan_id){
  $vars->{'form_action'} = 'tr_show_plan.cgi';
  print $cgi->header;
  $template->process("testopia/plan/choose.html.tmpl", $vars) 
      || ThrowTemplateError($template->error());
  exit;
}
validate_test_id($plan_id, 'plan');
push @{$::vars->{'style_urls'}}, 'testopia/css/default.css';

my $serverpush = support_server_push($cgi);

my $action = $cgi->param('action') || '';
$vars->{'action'} = "Commit";
$vars->{'form_action'} = "tr_show_plan.cgi";

####################
### Edit Actions ###
####################

### Archive or Unarchive ###
if ($action eq 'Archive' || $action eq 'Unarchive'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->canedit;
    do_update($plan);
    $vars->{'plan'} = $plan;
    $plan->toggle_archive(Bugzilla->user->id);
    $vars->{'tr_message'} = 
        $plan->isactive == 0 ? "Plan archived":"Plan Unarchived";
    print $cgi->header;
    display($plan);
        
}
#############
### Clone ###
#############
elsif ($action eq 'Clone'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->canedit;
    do_update($plan);
    $vars->{'plan'} = $plan;
    print $cgi->header;
    $template->process("testopia/plan/clone.html.tmpl", $vars) 
      || ThrowTemplateError($template->error());
    
}
elsif ($action eq 'do_clone'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->canedit;
    my $plan_name = $cgi->param('plan_name');
    if ($serverpush) {
        print $cgi->multipart_init();
        print $cgi->multipart_start();
    
        $template->process("list/server-push.html.tmpl", $vars)
          || ThrowTemplateError($template->error());

    }

    # All DB actions use place holders so we are OK doing this
    trick_taint($plan_name);
    my $newplanid = $plan->clone($plan_name, $cgi->param('copy_doc'));
    my $newplan = Bugzilla::Testopia::TestPlan->new($newplanid);
    if($cgi->param('copy_tags')){
        foreach my $tag (@{$plan->tags}){
            my $newtag = Bugzilla::Testopia::TestTag->new({
                           tag_name  => $tag->name
                         });
            my $newtagid = $newtag->store;
            $newplan->add_tag($newtagid);
        }
    }
    if ($cgi->param('copy_cases')){
        my @catids;
        #TODO: Copy case to the new category
        foreach my $id ($cgi->param('clone_categories')){
            detaint_natural($id);
            validate_selection($id,'category_id','test_case_categories');
            push @catids, $id;
        }
        my $caseids_ref = $plan->get_case_ids_by_category(\@catids); 
        my $progress_interval = 250;
        my $i = 0;
        my $total = scalar @$caseids_ref;
        foreach my $id (@$caseids_ref){
            $i++;
            if ($i % $progress_interval == 0 && $serverpush){
                print $cgi->multipart_end;
                print $cgi->multipart_start;
                $vars->{'complete'} = $i;
                $vars->{'total'} = $total;
                $template->process("testopia/progress.html.tmpl", $vars)
                  || ThrowTemplateError($template->error());
            } 

            my $case = Bugzilla::Testopia::TestCase->new($id);
            if ($cgi->param('copy_cases') == 2 ){
                my $caseid = $case->copy($newplan->id, 1);
                $case->link_plan($newplan->id, $caseid);
            }
            else {
                $case->link_plan($newplan->id);
            }
        }
    }
    if ($serverpush) {
        print $cgi->multipart_end;
        print $cgi->multipart_start;
    } else {
        print $cgi->header;
    }
    $vars->{'tr_message'} = "Plan ". $plan->name ." cloned as " . $newplan->name .".";
    $cgi->param('plan_id', $newplan->id);
    
    display($newplan);   
    print $cgi->multipart_final if $serverpush; 
}

### Changes to Plan Attributes or Doc ###
elsif ($action eq 'Commit'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->canedit;
    do_update($plan);
    $vars->{'tr_message'} = "Test plan updated";
    print $cgi->header;
    display($plan);    
}

elsif ($action eq 'Print'){
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-permission-denied", {'object' => 'plan'}) unless $plan->canview;
    $vars->{'plan'} = $plan; 
    print $cgi->header;   
    $template->process("testopia/plan/show-document.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
}

elsif ($action eq 'History'){
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-permission-denied", {'object' => 'plan'}) unless $plan->canview;
    $vars->{'plan'} = $plan; 
    $vars->{'diff'} = $plan->diff_plan_doc($cgi->param('new'),$cgi->param('old'));
    $vars->{'new'} = $cgi->param('new');
    $vars->{'old'} = $cgi->param('old');
    print $cgi->header;
    $template->process("testopia/plan/history.html.tmpl", $vars)
      || ThrowTemplateError($template->error());
       
}
#######################
### Add attachments ###
#######################
elsif ($action eq 'Attach'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->canedit;
    defined $cgi->upload('data')
        || ThrowUserError("file_not_specified");
    my $filename = $cgi->upload('data');       
    $cgi->param('description')
        || ThrowUserError("missing_attachment_description");
    my $description = $cgi->param('description');
    my $contenttype = $cgi->uploadInfo($cgi->param('data'))->{'Content-Type'};
    trick_taint($description);
    #trick_taint($contenttype);
    my $fh = $cgi->upload('data');
    my $data;
    # enable 'slurp' mode
    local $/;
    $data = <$fh>;       
    $data || ThrowUserError("zero_length_file");
    
    my $attachment = Bugzilla::Testopia::Attachment->new({
                        plan_id      => $plan_id,
                        submitter_id => Bugzilla->user->id,
                        description  => $description,
                        filename     => $filename,
                        mime_type    => $contenttype,
                        contents     => $data
    });

    $attachment->store;    
    $vars->{'tr_message'} = "Attachment added successfully";
    
    do_update($plan);
    print $cgi->header;
    display(Bugzilla::Testopia::TestPlan->new($plan_id));
}
#TODO: Import plans
elsif ($action eq 'import'){
    
}
elsif ($action eq 'Delete'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->candelete;
    $vars->{'plan'} = $plan;
    print $cgi->header;
    $template->process("testopia/plan/delete.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
    
}
elsif ($action eq 'do_delete'){
    Bugzilla->login(LOGIN_REQUIRED);
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-read-only", {'object' => 'plan'}) unless $plan->candelete;
    $plan->obliterate;
    $vars->{'deleted'} = 1;
    print $cgi->header;
    $template->process("testopia/plan/delete.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}
####################
### Ajax Actions ###
####################
elsif ($action eq 'getversions'){
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-permission-denied", {'object' => 'plan'}) unless $plan->canview;
    my $prod_id = $cgi->param("product_id");
    detaint_natural($prod_id);
    my $versions = $plan->get_product_versions($prod_id);
    my $ret;
    if ($cgi->param("product_id") == -1){
        # For update multiple from tr_list_plans
        $ret = "--Do Not Change--";
    }
    else{
        foreach my $v (@{$versions}){
            $ret .= $v->{'id'} . "|";
        }
        chop($ret);
    }
    print $ret;
}
elsif ($action eq 'caselist'){
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-permission-denied", {'object' => 'plan'}) unless $plan->canview;
    my $table = Bugzilla::Testopia::Table->new('case', 'tr_list_cases.cgi', $cgi, $plan->test_cases);
    $table->{'ajax'} = 1;
    $vars->{'table'} = $table;
    print $cgi->header;
    $template->process("testopia/case/table.html.tmpl", $vars)
        || ThrowTemplateError($template->error()); 
        
}
####################
### Just show it ###
####################
else{
    my $plan = Bugzilla::Testopia::TestPlan->new($plan_id);
    ThrowUserError("testopia-permission-denied", {'object' => 'plan'}) unless $plan->canview;
    print $cgi->header;
    display($plan);
}
###################
### Helper Subs ###
###################
sub do_update {
    my ($plan) = @_;

    my $newdoc = $cgi->param("plandoc");
    my $plan_name = trim($cgi->param('plan_name')) || '';
    my $product = $cgi->param('product_id') || '';
    my $prodver = $cgi->param('prod_version') || '';
    my $type = $cgi->param('type');

    ThrowUserError('testopia-missing-required-field', 
        {'field' => 'name'}) if ($plan_name eq '');
    ThrowUserError('testopia-missing-required-field', 
        {'field' => 'product'}) if ($product eq '');
    ThrowUserError('testopia-missing-required-field', 
        {'field' => 'product version'}) if ($prodver eq '');

    trick_taint($plan_name);
    trick_taint($prodver);

    detaint_natural($product);
    detaint_natural($type);
    
    validate_selection($type, 'type_id', 'test_plan_types');
    #TODO: 2.22 use Bugzilla::Product
    validate_selection($product, 'id', 'products');

    my $check = validate_version($prodver, $plan->product_id);
    if ($check ne '1'){
        $vars->{'tr_error'} = "Version mismatch. Please update the product version";
        $prodver = $check;
    }
    
    if($plan->diff_plan_doc($newdoc) ne ''){
        $plan->store_text($plan->id, Bugzilla->user->id, $newdoc);
    }
    
    my %newvalues = ( 
        'name'       => $plan_name,
        'product_id' => $product,
        'default_product_version' => $prodver,
        'type_id'    => $type
    );
    
    $plan->update(\%newvalues);
    
    # Add new tags 
    foreach my $tag_name (split(/[,]+/, $cgi->param('newtag'))){
        trick_taint($tag_name);
        my $tag = Bugzilla::Testopia::TestTag->new({tag_name => $tag_name});
        my $tag_id = $tag->store;
        $plan->add_tag($tag_id);
    }
    
    $cgi->delete_all;
    $cgi->param('plan_id', $plan->id);
}

sub display {
    my $plan = shift;
    my $casequery = new Bugzilla::CGI($cgi);
    my $runquery  = new Bugzilla::CGI($cgi);

    if (($cgi->param('order') || $cgi->param('page') || $cgi->param('viewall')) && $cgi->param('current_tab') eq 'case'){
        my $search = Bugzilla::Testopia::Search->new($cgi);
        my $table = Bugzilla::Testopia::Table->new('case', 'tr_show_plan.cgi', $cgi, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $case_query_limit}) if $table->list_count > $case_query_limit;
        
        $vars->{'case_table'} = $table;
        $runquery->delete('order');
        $runquery->delete('page');
        $runquery->delete('viewall');
        $runquery->param('current_tab', 'run');
        $search = Bugzilla::Testopia::Search->new($runquery);
        $table = Bugzilla::Testopia::Table->new('run', 'tr_show_plan.cgi', $runquery, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $run_query_limit}) if $table->list_count > $run_query_limit;
        $vars->{'run_table'} = $table;    

    }
    elsif (($cgi->param('order') || $cgi->param('page') || $cgi->param('viewall')) && $cgi->param('current_tab') eq 'run'){
        my $search = Bugzilla::Testopia::Search->new($cgi);
        my $table = Bugzilla::Testopia::Table->new('run', 'tr_show_plan.cgi', $cgi, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $run_query_limit}) if $table->list_count > $run_query_limit;
        
        $vars->{'run_table'} = $table;
        $casequery->delete('order');
        $casequery->delete('page');
        $casequery->delete('viewall');
        $casequery->param('current_tab', 'case');
        $search = Bugzilla::Testopia::Search->new($casequery);
        $table = Bugzilla::Testopia::Table->new('case', 'tr_show_plan.cgi', $casequery, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $case_query_limit}) if $table->list_count > $case_query_limit;
        $vars->{'case_table'} = $table;    
    }
    else {
    
        $casequery->param('current_tab', 'case');
        my $search = Bugzilla::Testopia::Search->new($casequery);
        my $table = Bugzilla::Testopia::Table->new('case', 'tr_show_plan.cgi', $casequery, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $case_query_limit}) if $table->list_count > $case_query_limit;
        $vars->{'case_table'} = $table;    
      
        $runquery->param('current_tab', 'run');
        $search = Bugzilla::Testopia::Search->new($runquery);
        $table = Bugzilla::Testopia::Table->new('run', 'tr_show_plan.cgi', $runquery, undef, $search->query);
        ThrowUserError('testopia-query-too-large', {'limit' => $run_query_limit}) if $table->list_count > $run_query_limit;
        $vars->{'run_table'} = $table;    
      
    }
    my @dojo_search;
    foreach my $run (@{$plan->test_runs}){
        push @dojo_search, "tip_" . $run->id;
    }
    push @dojo_search, "plandoc";
    $vars->{'dojo_search'} = objToJson(\@dojo_search);
    $vars->{'plan'} = $plan;
    
    $template->process("testopia/plan/show.html.tmpl", $vars) ||
        ThrowTemplateError($template->error());
}
