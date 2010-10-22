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
# Contributor(s): Myk Melez <myk@mozilla.org>
#                 Frédéric Buclin <LpSolit@gmail.com>

use strict;

package Bugzilla::FlagType;

=head1 NAME

Bugzilla::FlagType - A module to deal with Bugzilla flag types.

=head1 SYNOPSIS

FlagType.pm provides an interface to flag types as stored in Bugzilla.
See below for more information.

=head1 NOTES

=over

=item *

Use of private functions/variables outside this module may lead to
unexpected results after an upgrade.  Please avoid using private
functions in other files/modules.  Private functions are functions
whose names start with _ or are specifically noted as being private.

=back

=cut

use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Group;

use base qw(Bugzilla::Object);

###############################
####    Initialization     ####
###############################

use constant DB_TABLE => 'flagtypes';
use constant LIST_ORDER => 'sortkey, name';

use constant DB_COLUMNS => qw(
    id
    name
    description
    cc_list
    target_type
    sortkey
    is_active
    is_requestable
    is_requesteeble
    is_multiplicable
    grant_group_id
    request_group_id
);

use constant UPDATE_COLUMNS => qw(
    name
    description
    cc_list
    sortkey
    is_active
    is_requestable
    is_requesteeble
    is_multiplicable
    grant_group_id
    request_group_id
);

use constant VALIDATORS => {
    name             => \&_check_name,
    description      => \&_check_description,
    cc_list          => \&_check_cc_list,
    target_type      => \&_check_target_type,
    sortkey          => \&_check_sortey,
    is_active        => \&Bugzilla::Object::check_boolean,
    is_requestable   => \&Bugzilla::Object::check_boolean,
    is_requesteeble  => \&Bugzilla::Object::check_boolean,
    is_multiplicable => \&Bugzilla::Object::check_boolean,
    grant_group      => \&_check_group,
    request_group    => \&_check_group,
};

use constant UPDATE_VALIDATORS => {
    grant_group_id   => \&_check_group,
    request_group_id => \&_check_group,
};
###############################

sub create {
    my $class = shift;

    $class->check_required_create_fields(@_);
    my $params = $class->run_create_validators(@_);

    # Extract everything which is not a valid column name.
    $params->{grant_group_id} = delete $params->{grant_group};
    $params->{request_group_id} = delete $params->{request_group};
    my $inclusions = delete $params->{inclusions};
    my $exclusions = delete $params->{exclusions};

    my $flagtype = $class->insert_create_data($params);

    $flagtype->set_clusions({ inclusions => $inclusions,
                              exclusions => $exclusions });
    return $flagtype;
}

sub update {
    my $self = shift;
    my $dbh = Bugzilla->dbh;

    $dbh->bz_start_transaction();
    my $changes = $self->SUPER::update(@_);

    # Clear existing flags for bugs/attachments in categories no longer on
    # the list of inclusions or that have been added to the list of exclusions.
    my $flag_ids = $dbh->selectcol_arrayref('SELECT DISTINCT flags.id
                                               FROM flags
                                         INNER JOIN bugs
                                                 ON flags.bug_id = bugs.bug_id
                                    LEFT OUTER JOIN flaginclusions AS i
                                                 ON (flags.type_id = i.type_id
                                                     AND (bugs.product_id = i.product_id
                                                          OR i.product_id IS NULL)
                                                     AND (bugs.component_id = i.component_id
                                                          OR i.component_id IS NULL))
                                              WHERE flags.type_id = ?
                                                AND i.type_id IS NULL',
                                             undef, $self->id);
    Bugzilla::Flag->force_retarget($flag_ids);

    $flag_ids = $dbh->selectcol_arrayref('SELECT DISTINCT flags.id
                                            FROM flags
                                      INNER JOIN bugs
                                              ON flags.bug_id = bugs.bug_id
                                      INNER JOIN flagexclusions AS e
                                              ON flags.type_id = e.type_id
                                           WHERE flags.type_id = ?
                                             AND (bugs.product_id = e.product_id
                                                  OR e.product_id IS NULL)
                                             AND (bugs.component_id = e.component_id
                                                  OR e.component_id IS NULL)',
                                          undef, $self->id);
    Bugzilla::Flag->force_retarget($flag_ids);

    # Silently remove requestees from flags which are no longer
    # specifically requestable.
    if (!$self->is_requesteeble) {
        $dbh->do('UPDATE flags SET requestee_id = NULL WHERE type_id = ?',
                  undef, $self->id);
    }

    $dbh->bz_commit_transaction();
    return $changes;
}

###############################
####      Accessors      ######
###############################

=head2 METHODS

=over

=item C<id>

Returns the ID of the flagtype.

=item C<name>

Returns the name of the flagtype.

=item C<description>

Returns the description of the flagtype.

=item C<cc_list>

Returns the concatenated CC list for the flagtype, as a single string.

=item C<target_type>

Returns whether the flagtype applies to bugs or attachments.

=item C<is_active>

Returns whether the flagtype is active or disabled. Flags being
in a disabled flagtype are not deleted. It only prevents you from
adding new flags to it.

=item C<is_requestable>

Returns whether you can request for the given flagtype
(i.e. whether the '?' flag is available or not).

=item C<is_requesteeble>

Returns whether you can ask someone specifically or not.

=item C<is_multiplicable>

Returns whether you can have more than one flag for the given
flagtype in a given bug/attachment.

=item C<sortkey>

Returns the sortkey of the flagtype.

=back

=cut

sub id               { return $_[0]->{'id'};               }
sub name             { return $_[0]->{'name'};             }
sub description      { return $_[0]->{'description'};      }
sub cc_list          { return $_[0]->{'cc_list'};          }
sub target_type      { return $_[0]->{'target_type'} eq 'b' ? 'bug' : 'attachment'; }
sub is_active        { return $_[0]->{'is_active'};        }
sub is_requestable   { return $_[0]->{'is_requestable'};   }
sub is_requesteeble  { return $_[0]->{'is_requesteeble'};  }
sub is_multiplicable { return $_[0]->{'is_multiplicable'}; }
sub sortkey          { return $_[0]->{'sortkey'};          }
sub request_group_id { return $_[0]->{'request_group_id'}; }
sub grant_group_id   { return $_[0]->{'grant_group_id'};   }

################################
# Validators
################################

sub _check_name {
    my ($invocant, $name) = @_;

    $name = trim($name);
    ($name && $name !~ /[\s,]/ && length($name) <= 50)
      || ThrowUserError('flag_type_name_invalid', { name => $name });
    return $name;
}

sub _check_description {
    my ($invocant, $desc) = @_;

    $desc = trim($desc);
    $desc || ThrowUserError('flag_type_description_invalid');
    return $desc;
}

sub _check_cc_list {
    my ($invocant, $cc_list) = @_;

    length($cc_list) <= 200
      || ThrowUserError('flag_type_cc_list_invalid', { cc_list => $cc_list });

    my @addresses = split(/[,\s]+/, $cc_list);
    # We do not call Util::validate_email_syntax because these
    # addresses do not require to match 'emailregexp' and do not
    # depend on 'emailsuffix'. So we limit ourselves to a simple
    # sanity check:
    # - match the syntax of a fully qualified email address;
    # - do not contain any illegal character.
    foreach my $address (@addresses) {
        ($address =~ /^[\w\.\+\-=]+@[\w\.\-]+\.[\w\-]+$/
           && $address !~ /[\\\(\)<>&,;:"\[\] \t\r\n]/)
          || ThrowUserError('illegal_email_address',
                            {addr => $address, default => 1});
    }
    return $cc_list;
}

sub _check_target_type {
    my ($invocant, $target_type) = @_;

    ($target_type eq 'bug' || $target_type eq 'attachment')
      || ThrowCodeError('flag_type_target_type_invalid', { target_type => $target_type });
    return $target_type;
}

sub _check_sortey {
    my ($invocant, $sortkey) = @_;

    (detaint_natural($sortkey) && $sortkey <= MAX_SMALLINT)
      || ThrowUserError('flag_type_sortkey_invalid', { sortkey => $sortkey });
    return $sortkey;
}

sub _check_group {
    my ($invocant, $group) = @_;
    return unless $group;

    trick_taint($group);
    $group = Bugzilla::Group->check($group);
    return $group->id;
}

###############################
####       Methods         ####
###############################

sub set_name             { $_[0]->set('name', $_[1]); }
sub set_description      { $_[0]->set('description', $_[1]); }
sub set_cc_list          { $_[0]->set('cc_list', $_[1]); }
sub set_sortkey          { $_[0]->set('sortkey', $_[1]); }
sub set_is_active        { $_[0]->set('is_active', $_[1]); }
sub set_is_requestable   { $_[0]->set('is_requestable', $_[1]); }
sub set_is_specifically_requestable { $_[0]->set('is_requesteeble', $_[1]); }
sub set_is_multiplicable { $_[0]->set('is_multiplicable', $_[1]); }
sub set_grant_group      { $_[0]->set('grant_group_id', $_[1]); }
sub set_request_group    { $_[0]->set('request_group_id', $_[1]); }

sub set_clusions {
    my ($self, $list) = @_;
    my $dbh = Bugzilla->dbh;
    my $flag_id = $self->id;
    my %products;

    foreach my $category (keys %$list) {
        my $sth = $dbh->prepare("INSERT INTO flag$category 
                                (type_id, product_id, component_id) VALUES (?, ?, ?)");

        $dbh->do("DELETE FROM flag$category WHERE type_id = ?", undef, $flag_id);

        foreach my $prod_comp (@{$list->{$category} || []}) {
            my ($prod_id, $comp_id) = split(':', $prod_comp);
            # Does the product exist?
            if ($prod_id && detaint_natural($prod_id)) {
                $products{$prod_id} ||= new Bugzilla::Product($prod_id);
                next unless defined $products{$prod_id};

                # Does the component belong to this product?
                if ($comp_id && detaint_natural($comp_id)) {
                    my $found = grep { $_->id == $comp_id } @{$products{$prod_id}->components};
                    next unless $found;
                }
            }
            $prod_id ||= undef;
            $comp_id ||= undef;
            $sth->execute($flag_id, $prod_id, $comp_id);
        }
    }
}

=pod

=over

=item C<grant_list>

Returns a reference to an array of users who have permission to grant this flag type.
The arrays are populated with hashrefs containing the login, identity and visibility of users.

=item C<grant_group>

Returns the group (as a Bugzilla::Group object) in which a user
must be in order to grant or deny a request.

=item C<request_group>

Returns the group (as a Bugzilla::Group object) in which a user
must be in order to request or clear a flag.

=item C<flag_count>

Returns the number of flags belonging to the flagtype.

=item C<inclusions>

Return a hash of product/component IDs and names
explicitly associated with the flagtype.

=item C<exclusions>

Return a hash of product/component IDs and names
explicitly excluded from the flagtype.

=back

=cut

sub grant_list {
    my $self = shift;
    require Bugzilla::User;
    my @custusers;
    my @allusers = @{Bugzilla->user->get_userlist};
    foreach my $user (@allusers) {
        my $user_obj = new Bugzilla::User({name => $user->{login}});
        push(@custusers, $user) if $user_obj->can_set_flag($self);
    }
    return \@custusers;
}

sub grant_group {
    my $self = shift;

    if (!defined $self->{'grant_group'} && $self->{'grant_group_id'}) {
        $self->{'grant_group'} = new Bugzilla::Group($self->{'grant_group_id'});
    }
    return $self->{'grant_group'};
}

sub request_group {
    my $self = shift;

    if (!defined $self->{'request_group'} && $self->{'request_group_id'}) {
        $self->{'request_group'} = new Bugzilla::Group($self->{'request_group_id'});
    }
    return $self->{'request_group'};
}

sub flag_count {
    my $self = shift;

    if (!defined $self->{'flag_count'}) {
        $self->{'flag_count'} =
            Bugzilla->dbh->selectrow_array('SELECT COUNT(*) FROM flags
                                            WHERE type_id = ?', undef, $self->{'id'});
    }
    return $self->{'flag_count'};
}

sub inclusions {
    my $self = shift;

    if (!defined $self->{inclusions}) {
        ($self->{inclusions}, $self->{inclusions_as_hash}) = get_clusions($self->id, 'in');
    }
    return $self->{inclusions};
}

sub inclusions_as_hash {
    my $self = shift;

    $self->inclusions unless defined $self->{inclusions_as_hash};
    return $self->{inclusions_as_hash};
}

sub exclusions {
    my $self = shift;

    if (!defined $self->{exclusions}) {
        ($self->{exclusions}, $self->{exclusions_as_hash}) = get_clusions($self->id, 'ex');
    }
    return $self->{exclusions};
}

sub exclusions_as_hash {
    my $self = shift;

    $self->exclusions unless defined $self->{exclusions_as_hash};
    return $self->{exclusions_as_hash};
}

######################################################################
# Public Functions
######################################################################

=pod

=head1 PUBLIC FUNCTIONS/METHODS

=over

=item C<get_clusions($id, $type)>

Return a hash of product/component IDs and names
associated with the flagtype:
$clusions{'product_name:component_name'} = "product_ID:component_ID"

=back

=cut

sub get_clusions {
    my ($id, $type) = @_;
    my $dbh = Bugzilla->dbh;

    my $list =
        $dbh->selectall_arrayref("SELECT products.id, products.name, " .
                                 "       components.id, components.name " . 
                                 "FROM flagtypes, flag${type}clusions " . 
                                 "LEFT OUTER JOIN products " .
                                 "  ON flag${type}clusions.product_id = products.id " . 
                                 "LEFT OUTER JOIN components " .
                                 "  ON flag${type}clusions.component_id = components.id " . 
                                 "WHERE flagtypes.id = ? " .
                                 " AND flag${type}clusions.type_id = flagtypes.id",
                                 undef, $id);
    my (%clusions, %clusions_as_hash);
    foreach my $data (@$list) {
        my ($product_id, $product_name, $component_id, $component_name) = @$data;
        $product_id ||= 0;
        $product_name ||= "__Any__";
        $component_id ||= 0;
        $component_name ||= "__Any__";
        $clusions{"$product_name:$component_name"} = "$product_id:$component_id";
        $clusions_as_hash{$product_id}->{$component_id} = 1;
    }
    return (\%clusions, \%clusions_as_hash);
}

=pod

=over

=item C<match($criteria)>

Queries the database for flag types matching the given criteria
and returns a list of matching flagtype objects.

=back

=cut

sub match {
    my ($criteria) = @_;
    my $dbh = Bugzilla->dbh;

    # Depending on the criteria, we may have to append additional tables.
    my $tables = [DB_TABLE];
    my @criteria = sqlify_criteria($criteria, $tables);
    $tables = join(' ', @$tables);
    $criteria = join(' AND ', @criteria);

    my $flagtype_ids = $dbh->selectcol_arrayref("SELECT id FROM $tables WHERE $criteria");

    return Bugzilla::FlagType->new_from_list($flagtype_ids);
}

=pod

=over

=item C<count($criteria)>

Returns the total number of flag types matching the given criteria.

=back

=cut

sub count {
    my ($criteria) = @_;
    my $dbh = Bugzilla->dbh;

    # Depending on the criteria, we may have to append additional tables.
    my $tables = [DB_TABLE];
    my @criteria = sqlify_criteria($criteria, $tables);
    $tables = join(' ', @$tables);
    $criteria = join(' AND ', @criteria);

    my $count = $dbh->selectrow_array("SELECT COUNT(flagtypes.id)
                                       FROM $tables WHERE $criteria");
    return $count;
}

######################################################################
# Private Functions
######################################################################

=begin private

=head1 PRIVATE FUNCTIONS

=over

=item C<sqlify_criteria($criteria, $tables)>

Converts a hash of criteria into a list of SQL criteria.
$criteria is a reference to the criteria (field => value), 
$tables is a reference to an array of tables being accessed 
by the query.

=back

=cut

sub sqlify_criteria {
    my ($criteria, $tables) = @_;
    my $dbh = Bugzilla->dbh;

    # the generated list of SQL criteria; "1=1" is a clever way of making sure
    # there's something in the list so calling code doesn't have to check list
    # size before building a WHERE clause out of it
    my @criteria = ("1=1");

    if ($criteria->{name}) {
        my $name = $dbh->quote($criteria->{name});
        trick_taint($name); # Detaint data as we have quoted it.
        push(@criteria, "flagtypes.name = $name");
    }
    if ($criteria->{target_type}) {
        # The target type is stored in the database as a one-character string
        # ("a" for attachment and "b" for bug), but this function takes complete
        # names ("attachment" and "bug") for clarity, so we must convert them.
        my $target_type = $criteria->{target_type} eq 'bug'? 'b' : 'a';
        push(@criteria, "flagtypes.target_type = '$target_type'");
    }
    if (exists($criteria->{is_active})) {
        my $is_active = $criteria->{is_active} ? "1" : "0";
        push(@criteria, "flagtypes.is_active = $is_active");
    }
    if ($criteria->{product_id}) {
        my $product_id = $criteria->{product_id};
        
        # Add inclusions to the query, which simply involves joining the table
        # by flag type ID and target product/component.
        push(@$tables, "INNER JOIN flaginclusions AS i ON flagtypes.id = i.type_id");
        push(@criteria, "(i.product_id = $product_id OR i.product_id IS NULL)");
        
        # Add exclusions to the query, which is more complicated.  First of all,
        # we do a LEFT JOIN so we don't miss flag types with no exclusions.
        # Then, as with inclusions, we join on flag type ID and target product/
        # component.  However, since we want flag types that *aren't* on the
        # exclusions list, we add a WHERE criteria to use only records with
        # NULL exclusion type, i.e. without any exclusions.
        my $join_clause = "flagtypes.id = e.type_id ";

        my $addl_join_clause = "";
        if ($criteria->{component_id}) {
            my $component_id = $criteria->{component_id};
            push(@criteria, "(i.component_id = $component_id OR i.component_id IS NULL)");
            $join_clause .= "AND (e.component_id = $component_id OR e.component_id IS NULL) ";
        }
        else {
            $addl_join_clause = "AND e.component_id IS NULL OR (i.component_id != e.component_id) ";
        }
        $join_clause .= "AND ((e.product_id = $product_id $addl_join_clause) OR e.product_id IS NULL)";

        push(@$tables, "LEFT JOIN flagexclusions AS e ON ($join_clause)");
        push(@criteria, "e.type_id IS NULL");
    }
    if ($criteria->{group}) {
        my $gid = $criteria->{group};
        detaint_natural($gid);
        push(@criteria, "(flagtypes.grant_group_id = $gid " .
                        " OR flagtypes.request_group_id = $gid)");
    }
    
    return @criteria;
}

1;

=end private

=head1 SEE ALSO

=over

=item B<Bugzilla::Flags>

=back

=head1 CONTRIBUTORS

=over

=item Myk Melez <myk@mozilla.org>

=item Kevin Benton <kevin.benton@amd.com>

=item Frédéric Buclin <LpSolit@gmail.com>

=back

=cut
