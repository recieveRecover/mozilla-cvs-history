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
# Contributor(s): Marc Schumann <wurblzap@gmail.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>

package Bugzilla::WebService::Bug;

use strict;
use base qw(Bugzilla::WebService);
import SOAP::Data qw(type);

use Bugzilla::WebService::Constants;
use Bugzilla::Util qw(detaint_natural);
use Bugzilla::Bug;
use Bugzilla::BugMail;
use Bugzilla::Constants;

#############
# Constants #
#############

# This maps the names of internal Bugzilla bug fields to things that would
# make sense to somebody who's not intimately familiar with the inner workings
# of Bugzilla. (These are the field names that the WebService uses.)
use constant FIELD_MAP => {
    status      => 'bug_status',
    severity    => 'bug_severity',
    description => 'comment',
    summary     => 'short_desc',
    platform    => 'rep_platform',
};

###########
# Methods #
###########

sub get_bug {
    my $self = shift;
    my ($bug_id) = @_;

    Bugzilla->login;

    ValidateBugID($bug_id);
    return new Bugzilla::Bug($bug_id);
}


sub create {
    my ($self, $params) = @_;

    Bugzilla->login(LOGIN_REQUIRED);

    my %field_values;
    foreach my $field (keys %$params) {
        my $field_name = FIELD_MAP->{$field} || $field;
        $field_values{$field_name} = $params->{$field}; 
    }

    # Make sure all the required fields are in the hash.
    foreach my $field (Bugzilla::Bug::REQUIRED_CREATE_FIELDS) {
        $field_values{$field} = undef unless exists $field_values{$field};
    }

    # WebService users can't set the creation date of a bug.
    delete $field_values{'creation_ts'};

    my $bug = Bugzilla::Bug->create(\%field_values);

    Bugzilla::BugMail::Send($bug->bug_id, { changer => $bug->reporter->login });

    return { id => type('int')->value($bug->bug_id) };
}

1;

__END__

=head1 NAME

Bugzilla::Webservice::Bug - The API for creating, changing, and getting the
details of bugs.

=head1 DESCRIPTION

This part of the Bugzilla API allows you to file a new bug in Bugzilla.

=head1 METHODS

See L<Bugzilla::WebService> for a description of B<STABLE>, B<UNSTABLE>,
and B<EXPERIMENTAL>.

=over

=item C<create> B<EXPERIMENTAL>

=over

=item B<Description>

This allows you to create a new bug in Bugzilla. If you specify any
invalid fields, they will be ignored. If you specify any fields you
are not allowed to set, they will just be set to their defaults or ignored.

You cannot currently set all the items here that you can set on enter_bug.cgi.

The WebService interface may allow you to set things other than those listed
here, but realize that anything undocumented is B<UNSTABLE> and will very
likely change in the future.

=item B<Params>

Some params must be set, or an error will be thrown. These params are
marked B<Required>. 

Some parameters can have defaults set in Bugzilla, by the administrator.
If these parameters have defaults set, you can omit them. These parameters
are marked B<Defaulted>.

Clients that want to be able to interact uniformly with multiple
Bugzillas should always set both the params marked B<Required> and those 
marked B<Defaulted>, because some Bugzillas may not have defaults set
for B<Defaulted> parameters, and then this method will throw an error
if you don't specify them.

The descriptions of the parameters below are what they mean when Bugzilla is
being used to track software bugs. They may have other meanings in some
installations.

=over

=item C<product> (string) B<Required> - The name of the product the bug
is being filed against.

=item C<component> (string) B<Required> - The name of a component in the
product above.

=item C<summary> (string) B<Required> - A brief description of the bug being
filed.

=item C<version> (string) B<Required> - A version of the product above;
the version the bug was found in.

=item C<description> (string) B<Defaulted> - The initial description for 
this bug. Some Bugzilla installations require this to not be blank.

=item C<op_sys> (string) B<Defaulted> - The operating system the bug was
discovered on.

=item C<platform> (string) B<Defaulted> - What type of hardware the bug was
experienced on.

=item C<priority> (string) B<Defaulted> - What order the bug will be fixed
in by the developer, compared to the developer's other bugs.

=item C<severity> (string) B<Defaulted> - How severe the bug is.

=item C<alias> (string) - A brief alias for the bug that can be used 
instead of a bug number when accessing this bug. Must be unique in
all of this Bugzilla.

=item C<assigned_to> (username) - A user to assign this bug to, if you
don't want it to be assigned to the component owner.

=item C<cc> (array) - An array of usernames to CC on this bug.

=item C<qa_contact> (username) - If this installation has QA Contacts
enabled, you can set the QA Contact here if you don't want to use
the component's default QA Contact.

=item C<status> (string) - The status that this bug should start out as.
Note that only certain statuses can be set on bug creation.

=item C<target_milestone> (string) - A valid target milestone for this
product.

=back

In addition to the above parameters, if your installation has any custom
fields, you can set them just by passing in the name of the field and
its value as a string.

=item B<Returns>

A hash with one element, C<id>. This is the id of the newly-filed bug.

=item B<Errors>

=over

=item 103 (Invalid Alias)

The alias you specified is invalid for some reason. See the error message
for more details.

=item 104 (Invalid Field)

One of the drop-down fields has an invalid value. The error message will
have more detail.

=item 105 (Invalid Component)

Either you didn't specify a component, or the component you specified was
invalid.

=item 106 (Invalid Product)

Either you didn't specify a product, this product doesn't exist, or
you don't have permission to enter bugs in this product.

=item 107 (Invalid Summary)

You didn't specify a summary for the bug.

=item 504 (Invalid User)

Either the QA Contact, Assignee, or CC lists have some invalid user
in them. The error message will have more details.

=back

=back


=back
