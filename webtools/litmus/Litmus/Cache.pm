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
# The Original Code is Litmus.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): Zach Lipton <zach@zachlipton.com>

# Caching of product/group/subgroup data for litmusconfig.js

package Litmus::Cache;

use strict;

use Litmus;
use Data::JavaScript;
use Litmus::DB::Product;

our @ISA = qw(Exporter);
@Litmus::Cache::EXPORT = qw(
    rebuildCache
);

# generate a new litmusconfig.js file because something has updated:
sub rebuildCache {
    unless (-e "data") { system("mkdir", "data") }
    open(CACHE, ">data/litmusconfig.js.new");
    
    print CACHE "// Litmus configuration information\n";
    print CACHE "// Do not edit this file directly. It is autogenerated from the database\n";
    print CACHE "\n";
    
    # we build up @litmusconfig, a big perl data structure, and spit the 
    # whole thing out as javascript with Data::JavaScript
    # shield your eyes, we're in for a long trip
    my @litmusconfig;
    my @products = Litmus::DB::Product->retrieve_all();
    my $prodcount = 0;
    foreach my $curproduct (@products) {
        my $prodrow =  \%{$litmusconfig[$prodcount]};
           $prodrow->{"name"} = $curproduct->name();
           $prodrow->{"id"} = $curproduct->productid();
           
           my $brancharray = \@{$prodrow->{"branches"}};
           my $branchcount = 0;
           foreach my $curbranch ($curproduct->branches()) {
               my $branchrow = \%{$brancharray->[$branchcount]};
               $branchrow->{"name"} = $curbranch->name();
               $branchrow->{"id"} = $curbranch->branchid();
               $branchcount++;
           }
           
           my $platformarray = \@{$prodrow->{"platforms"}};
           my $platformcount = 0;
           foreach my $curplat ($curproduct->platforms()) {
               my $platrow = \%{$platformarray->[$platformcount]};
               $platrow->{"name"} = $curplat->name();
               $platrow->{"id"} = $curplat->platformid();
               
               my $opsysarray = \@{$platrow->{"opsyses"}};
               my $opsyscount = 0;
               foreach my $curopsys ($curplat->opsyses()) {
                   my $opsysrow = \%{$opsysarray->[$opsyscount]};
                   $opsysrow->{"name"} = $curopsys->name();
                   $opsysrow->{"id"} = $curopsys->opsysid();
                   $opsyscount++;
               }
               $platformcount++;
           }
           
           my $grouparray = \@{$prodrow->{"testgroups"}};
           my $groupcount = 0;
           foreach my $curgroup ($curproduct->testgroups()) {
               my $grouprow = \%{$grouparray->[$groupcount]};
               $grouprow->{"name"} = $curgroup->name();
               $grouprow->{"id"} = $curgroup->testgroupid();
               $groupcount++;
               
               my $subgrouparray = \@{$grouprow->{"subgroups"}};
               my $subgroupcount = 0;
               foreach my $cursubgroup ($curgroup->subgroups()) {
                   my $subgrouprow = \%{$subgrouparray->[$subgroupcount]};
                   $subgrouprow->{"name"} = $cursubgroup->name();
                   $subgrouprow->{"id"} = $cursubgroup->subgroupid();
                   $subgroupcount++;
               }
           }
           
        $prodcount++;
    }
        
    
    my $data = jsdump('litmusconfig', \@litmusconfig);
    $data =~ s/new Object;/new Array\(\);/g;
    print CACHE $data;
    close(CACHE);
    system("mv", "data/litmusconfig.js.new", "data/litmusconfig.js");
    system("chmod", "-R", "755", "data/");
}

1;