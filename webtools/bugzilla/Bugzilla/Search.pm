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
# Contributor(s): Gervase Markham <gerv@gerv.net>
#                 Terry Weissman <terry@mozilla.org>
#                 Dan Mosedale <dmose@mozilla.org>
#                 Stephan Niemz <st.n@gmx.net>
#                 Andreas Franke <afranke@mathweb.org>
#                 Myk Melez <myk@mozilla.org>
#                 Michael Schindler <michael@compressconsult.com>
#                 Max Kanat-Alexander <mkanat@bugzilla.org>
#                 Joel Peshkin <bugreport@peshkin.net>
#                 Lance Larsh <lance.larsh@oracle.com>
#                 Jesse Clark <jjclark1982@gmail.com>
#                 Rémi Zara <remi_zara@mac.com>
#                 Reed Loden <reed@reedloden.com>

use strict;

package Bugzilla::Search;
use base qw(Exporter);
@Bugzilla::Search::EXPORT = qw(
    EMPTY_COLUMN

    IsValidQueryType
    split_order_term
    translate_old_column
);

use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Constants;
use Bugzilla::Group;
use Bugzilla::User;
use Bugzilla::Field;
use Bugzilla::Status;
use Bugzilla::Keyword;

use Date::Format;
use Date::Parse;
use List::MoreUtils qw(uniq);
use Storable qw(dclone);

#############
# Constants #
#############

# If you specify a search type in the boolean charts, this describes
# which operator maps to which internal function here.
use constant OPERATORS => {
    equals         => \&_simple_operator,
    notequals      => \&_simple_operator,
    casesubstring  => \&_casesubstring,
    substring      => \&_substring,
    substr         => \&_substring,
    notsubstring   => \&_notsubstring,
    regexp         => \&_regexp,
    notregexp      => \&_notregexp,
    lessthan       => \&_simple_operator,
    lessthaneq     => \&_simple_operator,
    matches        => sub { ThrowUserError("search_content_without_matches"); },
    notmatches     => sub { ThrowUserError("search_content_without_matches"); },
    greaterthan    => \&_simple_operator,
    greaterthaneq  => \&_simple_operator,
    anyexact       => \&_anyexact,
    anywordssubstr => \&_anywordsubstr,
    allwordssubstr => \&_allwordssubstr,
    nowordssubstr  => \&_nowordssubstr,
    anywords       => \&_anywords,
    allwords       => \&_allwords,
    nowords        => \&_nowords,
    changedbefore  => \&_changedbefore_changedafter,
    changedafter   => \&_changedbefore_changedafter,
    changedfrom    => \&_changedfrom_changedto,
    changedto      => \&_changedfrom_changedto,
    changedby      => \&_changedby,
};

# Some operators are really just standard SQL operators, and are
# all implemented by the _simple_operator function, which uses this
# constant.
use constant SIMPLE_OPERATORS => {
    equals        => '=',
    notequals     => '!=',
    greaterthan   => '>',
    greaterthaneq => '>=',
    lessthan      => '<',
    lessthaneq    => "<=",
};

# Most operators just reverse by removing or adding "not" from/to them.
# However, some operators reverse in a different way, so those are listed
# here.
use constant OPERATOR_REVERSE => {
    nowords        => 'anywords',
    nowordssubstr  => 'anywordssubstr',
    anywords       => 'nowords',
    anywordssubstr => 'nowordssubstr',
    lessthan       => 'greaterthaneq',
    lessthaneq     => 'greaterthan',
    greaterthan    => 'lessthaneq',
    greaterthaneq  => 'lessthan',
    # The following don't currently have reversals:
    # casesubstring, anyexact, allwords, allwordssubstr
};

use constant OPERATOR_FIELD_OVERRIDE => {
    
    # User fields
    'attachments.submitter' => {
        _default => \&_attachments_submitter,
    },
    assigned_to => {
        _non_changed => \&_contact_nonchanged,
    },
    cc => {
        _non_changed => \&_cc_nonchanged,
    },
    commenter => {
        _default => \&_commenter,
    },
    reporter => {
        _non_changed => \&_contact_nonchanged,
    },
    'requestees.login_name' => {
        _default => \&_requestees_login_name,
    },
    'setters.login_name' => {
        _default => \&_setters_login_name,    
    },    
    qa_contact => {
        _non_changed => \&_qa_contact_nonchanged,
    },
    
    # General Bug Fields
    alias => {
        _non_changed => \&_alias_nonchanged,
    },
    'attach_data.thedata' => {
        _non_changed => \&_attach_data_thedata,
    },
    # We check all attachment fields against this.
    'attachments' => {
        _non_changed => \&_attachments,
    },
    blocked => {
        _non_changed => \&_blocked_nonchanged,
    },
    bug_group => {
        _non_changed => \&_bug_group_nonchanged,
    },
    classification => {
        _non_changed => \&_classification_nonchanged,
    },
    component => {
        _non_changed => \&_component_nonchanged,
    },
    content => {
        matches    => \&_content_matches,
        notmatches => \&_content_matches,
        _default   => sub { ThrowUserError("search_content_without_matches"); },
    },
    days_elapsed => {
        _default => \&_days_elapsed,
    },
    dependson => {
        _non_changed => \&_dependson_nonchanged,
    },
    keywords => {
        equals       => \&_keywords_exact,
        anyexact     => \&_keywords_exact,
        anyword      => \&_keywords_exact,
        allwords     => \&_keywords_exact,

        notequals     => \&_multiselect_negative,
        notregexp     => \&_multiselect_negative,
        notsubstring  => \&_multiselect_negative,
        nowords       => \&_multiselect_negative,
        nowordssubstr => \&_multiselect_negative,

        _non_changed  => \&_keywords_nonchanged,
    },
    'flagtypes.name' => {
        _default => \&_flagtypes_name,
    },    
    longdesc => {
        changedby     => \&_long_desc_changedby,
        changedbefore => \&_long_desc_changedbefore_after,
        changedafter  => \&_long_desc_changedbefore_after,
        _default      => \&_long_desc,
    },
    'longdescs.isprivate' => {
        _default => \&_longdescs_isprivate,
    },
    owner_idle_time => {
        greaterthan   => \&_owner_idle_time_greater_less,
        greaterthaneq => \&_owner_idle_time_greater_less,
        lessthan      => \&_owner_idle_time_greater_less,
        lessthaneq    => \&_owner_idle_time_greater_less,
        _default      => \&_invalid_combination,
    },
    
    product => {
        _non_changed => \&_product_nonchanged,
    },
    
    # Custom multi-select fields
    _multi_select => {
        notequals      => \&_multiselect_negative,
        notregexp      => \&_multiselect_negative,
        notsubstring   => \&_multiselect_negative,
        nowords        => \&_multiselect_negative,
        nowordssubstr  => \&_multiselect_negative,
        
        allwords       => \&_multiselect_multiple,
        allwordssubstr => \&_multiselect_multiple,
        anyexact       => \&_multiselect_multiple,
        
        _non_changed    => \&_multiselect_nonchanged,
    },
    
    # Timetracking Fields
    percentage_complete => {
        _non_changed => \&_percentage_complete,
    },
    work_time => {
        changedby     => \&_work_time_changedby,
        changedbefore => \&_work_time_changedbefore_after,
        changedafter  => \&_work_time_changedbefore_after,
        _default      => \&_work_time,
    },
    
};

# These are fields where special action is taken depending on the
# *value* passed in to the chart, sometimes.
use constant SPECIAL_PARSING => {
    # Pronoun Fields (Ones that can accept %user%, etc.)
    assigned_to => \&_contact_pronoun,
    cc          => \&_cc_pronoun,
    commenter   => \&_commenter_pronoun,
    qa_contact  => \&_contact_pronoun,
    reporter    => \&_contact_pronoun,
    
    # Date Fields that accept the 1d, 1w, 1m, 1y, etc. format.
    creation_ts => \&_timestamp_translate,
    deadline    => \&_timestamp_translate,
    delta_ts    => \&_timestamp_translate,
};

# Backwards compatibility for times that we changed the names of fields.
use constant FIELD_MAP => {
    changedin => 'days_elapsed',
    long_desc => 'longdesc',
};

# A SELECTed expression that we use as a placeholder if somebody selects
# <none> for the X, Y, or Z axis in report.cgi.
use constant EMPTY_COLUMN => '-1';

# Some fields are not sorted on themselves, but on other fields. 
# We need to have a list of these fields and what they map to.
use constant SPECIAL_ORDER => {
    'target_milestone' => {
        order => ['map_target_milestone.sortkey','map_target_milestone.value'],
        join  => {
            table => 'milestones',
            from  => 'target_milestone',
            to    => 'value',
            extra => ' AND bugs.product_id = map_target_milestone.product_id',
            join  => 'INNER',
        }
    },
};

# Certain columns require other columns to come before them
# in _select_columns, and should be put there if they're not there.
use constant COLUMN_DEPENDS => {
    classification      => ['product'],
    percentage_complete => ['actual_time', 'remaining_time'],
};

# This describes tables that must be joined when you want to display
# certain columns in the buglist. For the most part, Search.pm uses
# DB::Schema to figure out what needs to be joined, but for some
# fields it needs a little help.
use constant COLUMN_JOINS => {
    assigned_to => {
        from  => 'assigned_to',
        to    => 'userid',
        table => 'profiles',
        join  => 'INNER',
    },
    reporter => {
        from  => 'reporter',
        to    => 'userid',
        table => 'profiles',
        join  => 'INNER',
    },
    qa_contact => {
        from  => 'qa_contact',
        to    => 'userid',
        table => 'profiles',
    },
    component => {
        from  => 'component_id',
        to    => 'id',
        table => 'components',
        join  => 'INNER',
    },
    product => {
        from  => 'product_id',
        to    => 'id',
        table => 'products',
        join  => 'INNER',
    },
    classification => {
        table => 'classifications',
        from  => 'map_product.classification_id',
        to    => 'id',
        join  => 'INNER',
    },
    actual_time => {
        table  => 'longdescs',
        join   => 'INNER',
    },
    'flagtypes.name' => {
        name  => 'map_flags',
        table => 'flags',
        extra => ' AND attach_id IS NULL',
        then_to => {
            name  => 'map_flagtypes',
            table => 'flagtypes',
            from  => 'map_flags.type_id',
            to    => 'id',
        },
    },
    keywords => {
        table => 'keywords',
        then_to => {
            name  => 'map_keyworddefs',
            table => 'keyworddefs',
            from  => 'map_keywords.keywordid',
            to    => 'id',
        },
    },
};

# This constant defines the columns that can be selected in a query 
# and/or displayed in a bug list.  Column records include the following
# fields:
#
# 1. id: a unique identifier by which the column is referred in code;
#
# 2. name: The name of the column in the database (may also be an expression
#          that returns the value of the column);
#
# 3. title: The title of the column as displayed to users.
# 
# Note: There are a few hacks in the code that deviate from these definitions.
#       In particular, the redundant short_desc column is removed when the
#       client requests "all" columns.
#
# This is really a constant--that is, once it's been called once, the value
# will always be the same unless somebody adds a new custom field. But
# we have to do a lot of work inside the subroutine to get the data,
# and we don't want it to happen at compile time, so we have it as a
# subroutine.
sub COLUMNS {
    my $dbh = Bugzilla->dbh;
    my $cache = Bugzilla->request_cache;
    return $cache->{search_columns} if defined $cache->{search_columns};

    # These are columns that don't exist in fielddefs, but are valid buglist
    # columns. (Also see near the bottom of this function for the definition
    # of short_short_desc.)
    my %columns = (
        relevance            => { title => 'Relevance'  },
        assigned_to_realname => { title => 'Assignee'   },
        reporter_realname    => { title => 'Reporter'   },
        qa_contact_realname  => { title => 'QA Contact' },
    );

    # Next we define columns that have special SQL instead of just something
    # like "bugs.bug_id".
    my $actual_time = '(SUM(map_actual_time.work_time)'
        . ' * COUNT(DISTINCT map_actual_time.bug_when)/COUNT(bugs.bug_id))';
    my %special_sql = (
        deadline    => $dbh->sql_date_format('bugs.deadline', '%Y-%m-%d'),
        actual_time => $actual_time,

        percentage_complete =>
            "(CASE WHEN $actual_time + bugs.remaining_time = 0.0"
              . " THEN 0.0"
              . " ELSE 100"
                   . " * ($actual_time / ($actual_time + bugs.remaining_time))"
              . " END)",

        'flagtypes.name' => $dbh->sql_group_concat('DISTINCT ' 
            . $dbh->sql_string_concat('map_flagtypes.name', 'map_flags.status')),

        'keywords' => $dbh->sql_group_concat('DISTINCT map_keyworddefs.name'),
    );

    # Backward-compatibility for old field names. Goes new_name => old_name.
    # These are here and not in translate_old_column because the rest of the
    # code actually still uses the old names, while the fielddefs table uses
    # the new names (which is not the case for the fields handled by 
    # translate_old_column).
    my %old_names = (
        creation_ts => 'opendate',
        delta_ts    => 'changeddate',
        work_time   => 'actual_time',
    );

    # Fields that are email addresses
    my @email_fields = qw(assigned_to reporter qa_contact);
    # Other fields that are stored in the bugs table as an id, but
    # should be displayed using their name.
    my @id_fields = qw(product component classification);

    foreach my $col (@email_fields) {
        my $sql = "map_${col}.login_name";
        if (!Bugzilla->user->id) {
             $sql = $dbh->sql_string_until($sql, $dbh->quote('@'));
        }
        $special_sql{$col} = $sql;
        $columns{"${col}_realname"}->{name} = "map_${col}.realname";
    }

    foreach my $col (@id_fields) {
        $special_sql{$col} = "map_${col}.name";
    }

    # Do the actual column-getting from fielddefs, now.
    foreach my $field (Bugzilla->get_fields({ obsolete => 0, buglist => 1 })) {
        my $id = $field->name;
        $id = $old_names{$id} if exists $old_names{$id};
        my $sql;
        if (exists $special_sql{$id}) {
            $sql = $special_sql{$id};
        }
        elsif ($field->type == FIELD_TYPE_MULTI_SELECT) {
            $sql = $dbh->sql_group_concat(
                'DISTINCT map_' . $field->name . '.value');
        }
        else {
            $sql = 'bugs.' . $field->name;
        }
        $columns{$id} = { name => $sql, title => $field->description };
    }

    # The short_short_desc column is identical to short_desc
    $columns{'short_short_desc'} = $columns{'short_desc'};

    Bugzilla::Hook::process('buglist_columns', { columns => \%columns });

    $cache->{search_columns} = \%columns;
    return $cache->{search_columns};
}

sub REPORT_COLUMNS {
    my $columns = dclone(COLUMNS);
    # There's no reason to support reporting on unique fields.
    # Also, some other fields don't make very good reporting axises,
    # or simply don't work with the current reporting system.
    my @no_report_columns = 
        qw(bug_id alias short_short_desc opendate changeddate
           flagtypes.name keywords relevance);

    # Multi-select fields are not currently supported.
    my @multi_selects = Bugzilla->get_fields(
        { obsolete => 0, type => FIELD_TYPE_MULTI_SELECT });
    push(@no_report_columns, map { $_->name } @multi_selects);

    # If you're not a time-tracker, you can't use time-tracking
    # columns.
    if (!Bugzilla->user->is_timetracker) {
        push(@no_report_columns, TIMETRACKING_FIELDS);
    }

    foreach my $name (@no_report_columns) {
        delete $columns->{$name};
    }
    return $columns;
}

# These are fields that never go into the GROUP BY on any DB. bug_id
# is here because it *always* goes into the GROUP BY as the first item,
# so it should be skipped when determining extra GROUP BY columns.
use constant GROUP_BY_SKIP => EMPTY_COLUMN, qw(
    actual_time
    bug_id
    flagtypes.name
    keywords
    percentage_complete
);

######################
# Internal Accessors #
######################

# Fields that are legal for boolean charts of any kind.
sub _chart_fields {
    my ($self) = @_;

    if (!$self->{chart_fields}) {
        my $chart_fields = Bugzilla->fields({ by_name => 1 });

        if (!Bugzilla->user->is_timetracker) {
            foreach my $tt_field (TIMETRACKING_FIELDS) {
                delete $chart_fields->{$tt_field};
            }
        }
        $self->{chart_fields} = $chart_fields;
    }
    return $self->{chart_fields};
}

sub _multi_select_fields {
    my ($self) = @_;
    $self->{multi_select_fields} ||= Bugzilla->fields({
        by_name => 1,
        type    => [FIELD_TYPE_MULTI_SELECT, FIELD_TYPE_BUG_URLS]});
    return $self->{multi_select_fields};
}

##############################
# Internal Accessors: SELECT #
##############################

# These are the fields the user has chosen to display on the buglist,
# exactly as they were passed to new().
sub _input_columns { @{ $_[0]->{'fields'} || [] } }

# These are columns that are also going to be in the SELECT for one reason
# or another, but weren't actually requested by the caller.
sub _extra_columns {
    my ($self) = @_;
    # Everything that's going to be in the ORDER BY must also be
    # in the SELECT.
    $self->{extra_columns} ||= [ $self->_input_order_columns ];
    return @{ $self->{extra_columns} };
}

# For search functions to modify extra_columns. It doesn't matter if
# people push the same column onto this array multiple times, because
# _select_columns will call "uniq" on its final result.
sub _add_extra_column {
    my ($self, $column) = @_;
    push(@{ $self->{extra_columns} }, $column);
}

# These are the columns that we're going to be actually SELECTing.
sub _select_columns {
    my ($self) = @_;
    return @{ $self->{select_columns} } if $self->{select_columns};

    my @select_columns;
    foreach my $column ($self->_input_columns, $self->_extra_columns) {
        if (my $add_first = COLUMN_DEPENDS->{$column}) {
            push(@select_columns, @$add_first);
        }
        push(@select_columns, $column);
    }
    
    $self->{select_columns} = [uniq @select_columns];
    return @{ $self->{select_columns} };
}

# This takes _select_columns and translates it into the actual SQL that
# will go into the SELECT clause.
sub _sql_select {
    my ($self) = @_;
    my @sql_fields;
    foreach my $column ($self->_select_columns) {
        my $alias = $column;
        # Aliases cannot contain dots in them. We convert them to underscores.
        $alias =~ s/\./_/g;
        my $sql = ($column eq EMPTY_COLUMN)
                  ? EMPTY_COLUMN : COLUMNS->{$column}->{name} . " AS $alias";
        push(@sql_fields, $sql);
    }
    return @sql_fields;
}

################################
# Internal Accessors: ORDER BY #
################################

# The "order" that was requested by the consumer, exactly as it was
# requested.
sub _input_order { @{ $_[0]->{'order'} || [] } }
# The input order with just the column names, and no ASC or DESC.
sub _input_order_columns {
    my ($self) = @_;
    return map { (split_order_term($_))[0] } $self->_input_order;
}

# A hashref that describes all the special stuff that has to be done
# for various fields if they go into the ORDER BY clause.
sub _special_order {
    my ($self) = @_;
    return $self->{special_order} if $self->{special_order};
    
    my %special_order = %{ SPECIAL_ORDER() };
    my $select_fields = Bugzilla->fields({ type => FIELD_TYPE_SINGLE_SELECT });
    foreach my $field (@$select_fields) {
        next if $field->is_abnormal;
        my $name = $field->name;
        $special_order{$name} = {
            order => ["map_$name.sortkey", "map_$name.value"],
            join  => {
                table => $name,
                from  => "bugs.$name",
                to    => "value",
                join  => 'INNER',
            }
        };
    }
    $self->{special_order} = \%special_order;
    return $self->{special_order};
}

sub _sql_order_by {
    my ($self) = @_;
    if (!$self->{sql_order_by}) {
        my @order_by = map { $self->_translate_order_by_column($_) }
                           $self->_input_order;
        $self->{sql_order_by} = \@order_by;
    }
    return @{ $self->{sql_order_by} };
}

############################
# Internal Accessors: FROM #
############################

# JOIN statements for the SELECT and ORDER BY columns. This should not be called
# Until the moment it is needed, because _select_columns might be
# modified by the charts.
sub _select_order_joins {
    my ($self) = @_;
    my @joins;
    foreach my $field ($self->_select_columns) {
        my @column_join = $self->_column_join($field);
        push(@joins, @column_join);
    }
    foreach my $field ($self->_input_order_columns) {
        my $join_info = $self->_special_order->{$field}->{join};
        if ($join_info) {
            my @join_sql = $self->_translate_join($field, $join_info);
            push(@joins, @join_sql);
        }
    }
    return @joins;
}

################################
# Internal Accessors: GROUP BY #
################################

# And these are the fields that we have to do GROUP BY for in DBs
# that are more strict about putting everything into GROUP BY.
sub _sql_group_by {
    my ($self) = @_;

    # Strict DBs require every element from the SELECT to be in the GROUP BY,
    # unless that element is being used in an aggregate function.
    my @extra_group_by;
    foreach my $column ($self->_select_columns) {
        next if $self->_skip_group_by->{$column};
        my $sql = COLUMNS->{$column}->{name};
        push(@extra_group_by, $sql);
    }

    # And all items from ORDER BY must be in the GROUP BY. The above loop 
    # doesn't catch items that were put into the ORDER BY from SPECIAL_ORDER.
    foreach my $column ($self->_input_order_columns) {
        my $special_order = $self->_special_order->{$column}->{order};
        next if !$special_order;
        push(@extra_group_by, @$special_order);
    }
    
    @extra_group_by = uniq @extra_group_by;
    
    # bug_id is the only field we actually group by.
    return ('bugs.bug_id', join(',', @extra_group_by));
}

# A helper for _sql_group_by.
sub _skip_group_by {
    my ($self) = @_;
    return $self->{skip_group_by} if $self->{skip_group_by};
    my @skip_list = GROUP_BY_SKIP;
    push(@skip_list, keys %{ $self->_multi_select_fields });
    my %skip_hash = map { $_ => 1 } @skip_list;
    $self->{skip_group_by} = \%skip_hash;
    return $self->{skip_group_by};
}

##############################################
# Internal Accessors: Special Params Parsing #
##############################################

sub _params { $_[0]->{params} }

sub _parse_params {
    my ($self) = @_;
    $self->_special_parse_bug_status();
    $self->_special_parse_resolution();
    my @charts = $self->_parse_basic_fields();
    push(@charts, $self->_special_parse_email());
    push(@charts, $self->_special_parse_chfield());
    push(@charts, $self->_special_parse_deadline());
    return @charts;
}

sub _parse_basic_fields {
    my ($self) = @_;
    my $params = $self->_params;
    my $chart_fields = $self->_chart_fields;
    
    foreach my $old_name (keys %{ FIELD_MAP() }) {
        if (defined $params->param($old_name)) {
            my @value = $params->param($old_name);
            $params->delete($old_name);
            my $new_name = FIELD_MAP->{$old_name};
            $params->param($new_name, @value);
        }
    }
    
    my @charts;
    foreach my $field_name (keys %$chart_fields) {
        # CGI params shouldn't have periods in them, so we only accept
        # period-separated fields with underscores where the periods go.
        my $param_name = $field_name;
        $param_name =~ s/\./_/g;
        next if !defined $params->param($param_name);
        my $operator = $params->param("${param_name}_type");
        $operator = 'anyexact' if !$operator;
        $operator = 'matches' if $operator eq 'content';
        my $string_value = join(',', $params->param($param_name));
        push(@charts, [$field_name, $operator, $string_value]);
    }
    return @charts;
}

sub _special_parse_bug_status {
    my ($self) = @_;
    my $params = $self->_params;
    return if !defined $params->param('bug_status');

    my @bug_status = $params->param('bug_status');
    # Also include inactive bug statuses, as you can query them.
    my $legal_statuses = $self->_chart_fields->{'bug_status'}->legal_values;

    # If the status contains __open__ or __closed__, translate those
    # into their equivalent lists of open and closed statuses.
    if (grep { $_ eq '__open__' } @bug_status) {
        my @open = grep { $_->is_open } @$legal_statuses;
        @open = map { $_->name } @open;
        push(@bug_status, @open);
    }
    if (grep { $_ eq '__closed__' } @bug_status) {
        my @closed = grep { not $_->is_open } @$legal_statuses;
        @closed = map { $_->name } @closed;
        push(@bug_status, @closed);
    }

    @bug_status = uniq @bug_status;
    my $all = grep { $_ eq "__all__" } @bug_status;
    # This will also handle removing __open__ and __closed__ for us
    # (__all__ too, which is why we check for it above, first).
    @bug_status = _valid_values(\@bug_status, $legal_statuses);

    # If the user has selected every status, change to selecting none.
    # This is functionally equivalent, but quite a lot faster.    
    if ($all or scalar(@bug_status) == scalar(@$legal_statuses)) {
        $params->delete('bug_status');
    }
    else {
        $params->param('bug_status', @bug_status);
    }
}

sub _special_parse_chfield {
    my ($self) = @_;
    my $params = $self->_params;
    
    my $date_from = trim(lc($params->param('chfieldfrom')));
    $date_from = '' if !defined $date_from;
    my $date_to = trim(lc($params->param('chfieldto')));
    $date_to = '' if !defined $date_to;
    $date_from = '' if $date_from eq 'now';
    $date_to = '' if $date_to eq 'now';
    my @fields = $params->param('chfield');
    my $value_to = trim($params->param('chfieldvalue'));
    $value_to = '' if !defined $value_to;

    my @charts;
    # It is always safe and useful to push delta_ts into the charts
    # if there are any dates specified. It doesn't conflict with
    # searching [Bug creation], because a bug's delta_ts is set to
    # its creation_ts when it is created. So this just gives the
    # database an additional index to possibly choose.
    if ($date_from ne '') {
        push(@charts, ['delta_ts', 'greaterthaneq', $date_from]);
    }
    if ($date_to ne '') {
        push(@charts, ['delta_ts', 'lessthaneq', $date_to]);
    }
    
    if (grep { $_ eq '[Bug creation]' } @fields) {
        if ($date_from ne '') {
            push(@charts, ['creation_ts', 'greaterthaneq', $date_from]);
        }
        if ($date_to ne '') {
            push(@charts, ['creation_ts', 'lessthaneq', $date_to]);
        }
    }

    # Basically, we construct the chart like:
    #
    # (added_for_field1 = value OR added_for_field2 = value)
    # AND (date_field1_changed >= date_from OR date_field2_changed >= date_from)
    # AND (date_field1_changed <= date_to OR date_field2_changed <= date_to)
    #
    # Theoretically, all we *really* would need to do is look for the field id
    # in the bugs_activity table, because we've already limited the search
    # by delta_ts above, but there's no chart to do that, so we check the
    # change date of the fields.
    
    if ($value_to ne '') {
        my @value_chart;
        foreach my $field (@fields) {
            next if $field eq '[Bug creation]';
            push(@value_chart, $field, 'changedto', $value_to);
        }
        push(@charts, \@value_chart) if @value_chart;
    }

    if ($date_from ne '') {
        my @date_from_chart;
        foreach my $field (@fields) {
            next if $field eq '[Bug creation]';
            push(@date_from_chart, $field, 'changedafter', $date_from);
        }
        push(@charts, \@date_from_chart) if @date_from_chart;
    }
    if ($date_to ne '') {
        my @date_to_chart;
        foreach my $field (@fields) {
            push(@date_to_chart, $field, 'changedbefore', $date_to);
        }
        push(@charts, \@date_to_chart) if @date_to_chart;
    }

    return @charts;
}

sub _special_parse_deadline {
    my ($self) = @_;
    return if !Bugzilla->user->is_timetracker;
    my $params = $self->_params;
    
    my @charts;
    if (my $from = $params->param('deadlinefrom')) {
        push(@charts, ['deadline', 'greaterthaneq', $from]);
    }
    if (my $to = $params->param('deadlineto')) {
        push(@charts, ['deadline', 'lessthaneq', $to]);
    }
      
    return @charts;    
}

sub _special_parse_email {
    my ($self) = @_;
    my $params = $self->_params;
    
    my @email_params = grep { $_ =~ /^email\d+$/ } $params->param();
    
    my @charts;
    foreach my $param (@email_params) {
        $param =~ /(\d+)$/;
        my $id = $1;
        my $email = trim($params->param("email$id"));
        next if $email eq "";
        my $type = $params->param("emailtype$id");
        $type = "anyexact" if $type eq "exact";

        my @or_charts;
        foreach my $field qw(assigned_to reporter cc qa_contact) {
            if ($params->param("email$field$id")) {
                push(@or_charts, $field, $type, $email);
            }
        }
        if ($params->param("emaillongdesc$id")) {
            push(@or_charts, "commenter", $type, $email);
        }

        push(@charts, \@or_charts);
    }
    
    return @charts;
}

sub _special_parse_resolution {
    my ($self) = @_;
    my $params = $self->_params;
    return if !defined $params->param('resolution');

    my @resolution = $params->param('resolution');
    my $legal_resolutions = $self->_chart_fields->{resolution}->legal_values;
    @resolution = _valid_values(\@resolution, $legal_resolutions, '---');
    if (scalar(@resolution) == scalar(@$legal_resolutions)) {
        $params->delete('resolution');
    }
}

##################################
# Helpers for Internal Accessors #
##################################

sub _column_join {
    my ($self, $field) = @_;
    my $join_info = COLUMN_JOINS->{$field};
    if (!$join_info) {
        if ($self->_multi_select_fields->{$field}) {
            return $self->_translate_join($field, { table => "bug_$field" });
        }
        return ();
    }
    return $self->_translate_join($field, $join_info);
}

sub _valid_values {
    my ($input, $valid, $extra_value) = @_;
    my @result;
    foreach my $item (@$input) {
        if (defined $extra_value and $item eq $extra_value) {
            push(@result, $item);
        }
        elsif (grep { $_->name eq $item } @$valid) {
            push(@result, $item);
        }
    }
    return @result;
}

sub _translate_join {
    my ($self, $field, $join_info) = @_;
    my $from_table = "bugs";
    my $from  = $join_info->{from} || "bug_id";
    if ($from =~ /^(\w+)\.(\w+)$/) {
        ($from_table, $from) = ($1, $2);
    }
    my $to    = $join_info->{to}    || "bug_id";
    my $join  = $join_info->{join}  || 'LEFT';
    my $table = $join_info->{table};
    die "$field requires a table in COLUMN_JOINS" if !$table;
    my $extra = $join_info->{extra} || '';
    my $name  = $join_info->{name}  || "map_$field";
    $name =~ s/\./_/g;

    my @join_sql = "$join JOIN $table AS $name"
                        . " ON $from_table.$from = $name.$to$extra";
    if (my $then_to = $join_info->{then_to}) {
        push(@join_sql, $self->_translate_join($field, $then_to));
    }
    return @join_sql;
}

sub _translate_order_by_column {
    my ($self, $order_by_item) = @_;

    my ($field, $direction) = split_order_term($order_by_item);
    
    $direction = '' if lc($direction) eq 'asc';
    my $special_order = $self->_special_order->{$field}->{order};
    # Standard fields have underscores in their SELECT alias instead
    # of a period (because aliases can't have periods).
    $field =~ s/\./_/g;
    my @items = $special_order ? @$special_order : $field;
    if (lc($direction) eq 'desc') {
        @items = map { "$_ DESC" } @items;
    }
    return @items;
}

###############
# Constructor #
###############

# Create a new Search
# Note that the param argument may be modified by Bugzilla::Search
sub new {
    my $invocant = shift;
    my $class = ref($invocant) || $invocant;
  
    my $self = { @_ };
    bless($self, $class);

    $self->init();
 
    return $self;
}

sub init {
    my $self = shift;
    my $params = $self->_params;
    $params->convert_old_params();
    $self->{'user'} ||= Bugzilla->user;
    my $user = $self->{'user'};

    my @supptables;
    my @wherepart;
    my @having;
    my @andlist;

    my $dbh = Bugzilla->dbh;

    my @specialchart = $self->_parse_params();
    
    foreach my $f ("short_desc", "longdesc", "bug_file_loc",
                   "status_whiteboard") {
        if (defined $params->param($f)) {
            my $s = trim($params->param($f));
            if ($s ne "") {
                my $type = $params->param($f . "_type");
                push(@specialchart, [$f, $type, $s]);
            }
        }
    }

    # first we delete any sign of "Chart #-1" from the HTML form hash
    # since we want to guarantee the user didn't hide something here
    my @badcharts = grep /^(field|type|value)-1-/, $params->param();
    foreach my $field (@badcharts) {
        $params->delete($field);
    }

    # now we take our special chart and stuff it into the form hash
    my $chart = -1;
    my $row = 0;
    foreach my $ref (@specialchart) {
        my $col = 0;
        while (@$ref) {
            $params->param("field$chart-$row-$col", shift(@$ref));
            $params->param("type$chart-$row-$col", shift(@$ref));
            $params->param("value$chart-$row-$col", shift(@$ref));
            $col++;

        }
        $row++;
    }


# A boolean chart is a way of representing the terms in a logical
# expression.  Bugzilla builds SQL queries depending on how you enter
# terms into the boolean chart. Boolean charts are represented in
# urls as tree-tuples of (chart id, row, column). The query form
# (query.cgi) may contain an arbitrary number of boolean charts where
# each chart represents a clause in a SQL query.
#
# The query form starts out with one boolean chart containing one
# row and one column.  Extra rows can be created by pressing the
# AND button at the bottom of the chart.  Extra columns are created
# by pressing the OR button at the right end of the chart. Extra
# charts are created by pressing "Add another boolean chart".
#
# Each chart consists of an arbitrary number of rows and columns.
# The terms within a row are ORed together. The expressions represented
# by each row are ANDed together. The expressions represented by each
# chart are ANDed together.
#
#        ----------------------
#        | col2 | col2 | col3 |
# --------------|------|------|
# | row1 |  a1  |  a2  |      |
# |------|------|------|------|  => ((a1 OR a2) AND (b1 OR b2 OR b3) AND (c1))
# | row2 |  b1  |  b2  |  b3  |
# |------|------|------|------|
# | row3 |  c1  |      |      |
# -----------------------------
#
#        --------
#        | col2 |
# --------------|
# | row1 |  d1  | => (d1)
# ---------------
#
# Together, these two charts represent a SQL expression like this
# SELECT blah FROM blah WHERE ( (a1 OR a2)AND(b1 OR b2 OR b3)AND(c1)) AND (d1)
#
# The terms within a single row of a boolean chart are all constraints
# on a single piece of data.  If you're looking for a bug that has two
# different people cc'd on it, then you need to use two boolean charts.
# This will find bugs with one CC matching 'foo@blah.org' and and another
# CC matching 'bar@blah.org'.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# --------------------------------------------------------------
# CC    | equal to
# bar@blah.org
#
# If you try to do this query by pressing the AND button in the
# original boolean chart then what you'll get is an expression that
# looks for a single CC where the login name is both "foo@blah.org",
# and "bar@blah.org". This is impossible.
#
# --------------------------------------------------------------
# CC    | equal to
# foo@blah.org
# AND
# CC    | equal to
# bar@blah.org
# --------------------------------------------------------------

# $chartid is the number of the current chart whose SQL we're constructing
# $row is the current row of the current chart

# names for table aliases are constructed using $chartid and $row
#   SELECT blah  FROM $table "$table_$chartid_$row" WHERE ....

# $f  = field of table in bug db (e.g. bug_id, reporter, etc)
# $ff = qualified field name (field name prefixed by table)
#       e.g. bugs_activity.bug_id
# $t  = type of query. e.g. "equal to", "changed after", case sensitive substr"
# $v  = value - value the user typed in to the form
# $q  = sanitized version of user input trick_taint(($dbh->quote($v)))
# @supptables = Tables and/or table aliases used in query
# %suppseen   = A hash used to store all the tables in supptables to weed
#               out duplicates.
# @supplist   = A list used to accumulate all the JOIN clauses for each
#               chart to merge the ON sections of each.
# $suppstring = String which is pasted into query containing all table names

    my $sequence = 0;
    $row = 0;
    for ($chart=-1 ;
         $chart < 0 || $params->param("field$chart-0-0") ;
         $chart++) 
    {
        my $chartid = $chart >= 0 ? $chart : "";
        my @chartandlist;
        for ($row = 0 ;
             $params->param("field$chart-$row-0") ;
             $row++) 
        {
            my @orlist;
            for (my $col = 0 ;
                 $params->param("field$chart-$row-$col") ;
                 $col++) 
            {
                my $field = $params->param("field$chart-$row-$col") || "noop";
                my $operator = $params->param("type$chart-$row-$col") || "noop";
                my $value = $params->param("value$chart-$row-$col");
                $value = "" if !defined $value;
                $value = trim($value);
                next if ($field eq "noop" || $operator eq "noop" 
                         || $value eq "");

                # chart -1 is generated by other code above, not from the user-
                # submitted form, so we'll blindly accept any values in chart -1
                if (!$self->_chart_fields->{$field} and $chart != -1) {
                    ThrowCodeError("invalid_field_name", { field => $field });
                }

                # This is either from the internal chart (in which case we
                # already know about it), or it was in _chart_fields, so it is
                # a valid field name, which means that it's ok.
                trick_taint($field);
                my $quoted = $dbh->quote($value);
                trick_taint($quoted);

                my $full_field = $field =~ /\./ ? $field : "bugs.$field";
                my %search_args = (
                    chart_id   => $chartid,
                    sequence   => $sequence,
                    field      => $field,
                    full_field => $full_field,
                    operator   => $operator,
                    value      => $value,
                    quoted     => $quoted,
                    joins        => \@supptables,
                    where        => \@wherepart,
                    having       => \@having,
                );
                # This should add a "term" selement to %search_args.
                $self->do_search_function(\%search_args);
                
                if ($search_args{term}) {
                    # All the things here that don't get pulled out of
                    # %search_args are their original values before
                    # do_search_function modified them.
                    $self->search_description({
                        field => $field, type => $operator,
                        value => $value, term => $search_args{term},
                    });
                    push(@orlist, $search_args{term});
                }
                else {
                    # This field and this type don't work together.
                    ThrowUserrror("search_field_operator_invalid",
                                   { field => $field, operator => $operator });
                }
            }
            if (@orlist) {
                @orlist = map("($_)", @orlist) if (scalar(@orlist) > 1);
                push(@chartandlist, "(" . join(" OR ", @orlist) . ")");
            }
        }
        if (@chartandlist) {
            if ($params->param("negate$chart")) {
                push(@andlist, "NOT(" . join(" AND ", @chartandlist) . ")");
            } else {
                push(@andlist, "(" . join(" AND ", @chartandlist) . ")");
            }
        }
    }

    my %suppseen = ("bugs" => 1);
    my $suppstring = "bugs";
    my @supplist = (" ");
    foreach my $str ($self->_select_order_joins, @supptables) {

        if ($str =~ /^(LEFT|INNER|RIGHT)\s+JOIN/i) {
            $str =~ /^(.*?)\s+ON\s+(.*)$/i;
            my ($leftside, $rightside) = ($1, $2);
            if (defined $suppseen{$leftside}) {
                $supplist[$suppseen{$leftside}] .= " AND ($rightside)";
            } else {
                $suppseen{$leftside} = scalar @supplist;
                push @supplist, " $leftside ON ($rightside)";
            }
        } else {
            # Do not accept implicit joins using comma operator
            # as they are not DB agnostic
            ThrowCodeError("comma_operator_deprecated");
        }
    }
    $suppstring .= join('', @supplist);
    
    # Make sure we create a legal SQL query.
    @andlist = ("1 = 1") if !@andlist;


    my $query = "SELECT " . join(', ', $self->_sql_select) .
                " FROM $suppstring" .
                " LEFT JOIN bug_group_map " .
                " ON bug_group_map.bug_id = bugs.bug_id ";

    if ($user->id) {
        if (scalar @{ $user->groups }) {
            $query .= " AND bug_group_map.group_id NOT IN (" 
                   . $user->groups_as_string . ") ";
        }

        $query .= " LEFT JOIN cc ON cc.bug_id = bugs.bug_id AND cc.who = " . $user->id;
    }

    $query .= " WHERE " . join(' AND ', (@wherepart, @andlist)) .
              " AND bugs.creation_ts IS NOT NULL AND ((bug_group_map.group_id IS NULL)";

    if ($user->id) {
        my $userid = $user->id;
        $query .= "    OR (bugs.reporter_accessible = 1 AND bugs.reporter = $userid) " .
              "    OR (bugs.cclist_accessible = 1 AND cc.who IS NOT NULL) " .
              "    OR (bugs.assigned_to = $userid) ";
        if (Bugzilla->params->{'useqacontact'}) {
            $query .= "OR (bugs.qa_contact = $userid) ";
        }
    }

    $query .= ") " . $dbh->sql_group_by($self->_sql_group_by);


    if (@having) {
        $query .= " HAVING " . join(" AND ", @having);
    }

    if ($self->_sql_order_by) {
        $query .= " ORDER BY " . join(',', $self->_sql_order_by);
    }

    $self->{'sql'} = $query;
}

###############################################################################
# Helper functions for the init() method.
###############################################################################

# This takes information about the current boolean chart and translates
# it into SQL, using the constants at the top of this file.
sub do_search_function {
    my ($self, $args) = @_;
    my ($field, $operator, $value) = @$args{qw(field operator value)};
    
    my $actual_field = FIELD_MAP->{$field} || $field;
    $args->{field} = $actual_field;
    
    if (my $parse_func = SPECIAL_PARSING->{$actual_field}) {
        $self->$parse_func($args);
        # Some parsing functions set $term, though most do not.
        # For the ones that set $term, we don't need to do any further
        # parsing.
        return if $args->{term};
    }
    
    my $override = OPERATOR_FIELD_OVERRIDE->{$actual_field};
    if (!$override) {
        # Multi-select fields get special handling.
        if ($self->_multi_select_fields->{$actual_field}) {
            $override = OPERATOR_FIELD_OVERRIDE->{_multi_select};
        }
        # And so do attachment fields, if they don't have a specific
        # individual override.
        elsif ($actual_field =~ /^attachments\./) {
            $override = OPERATOR_FIELD_OVERRIDE->{attachments};
        }
    }
    
    if ($override) {
        my $search_func = $self->_pick_override_function($override, $operator);
        $self->$search_func($args) if $search_func;
    }

    # Some search functions set $term, and some don't. For the ones that
    # don't (or for fields that don't have overrides) we now call the
    # direct operator function from OPERATORS.
    if (!defined $args->{term}) {
        $self->_do_operator_function($args);
    }
}

# A helper for various search functions that need to run operator
# functions directly.
sub _do_operator_function {
    my ($self, $func_args) = @_;
    my $operator = $func_args->{operator};
    my $operator_func = OPERATORS->{$operator};
    $self->$operator_func($func_args);
}

sub _reverse_operator {
    my ($self, $operator) = @_;
    my $reverse = OPERATOR_REVERSE->{$operator};
    return $reverse if $reverse;
    if ($operator =~ s/^not//) {
        return $operator;
    }
    return "not$operator";
}

sub _pick_override_function {
    my ($self, $override, $operator) = @_;
    my $search_func = $override->{$operator};

    if (!$search_func) {
        # If we don't find an override for one specific operator,
        # then there are some special override types:
        # _non_changed: For any operator that doesn't have the word
        #               "changed" in it
        # _default: Overrides all operators that aren't explicitly specified.
        if ($override->{_non_changed} and $operator !~ /changed/) {
            $search_func = $override->{_non_changed};
        }
        elsif ($override->{_default}) {
            $search_func = $override->{_default};
        }
    }

    return $search_func;
}


sub SqlifyDate {
    my ($str) = @_;
    $str = "" if !defined $str;
    if ($str eq "") {
        my ($sec, $min, $hour, $mday, $month, $year, $wday) = localtime(time());
        return sprintf("%4d-%02d-%02d 00:00:00", $year+1900, $month+1, $mday);
    }


    if ($str =~ /^(-|\+)?(\d+)([hHdDwWmMyY])$/) {   # relative date
        my ($sign, $amount, $unit, $date) = ($1, $2, lc $3, time);
        my ($sec, $min, $hour, $mday, $month, $year, $wday)  = localtime($date);
        if ($sign && $sign eq '+') { $amount = -$amount; }
        if ($unit eq 'w') {                  # convert weeks to days
            $amount = 7*$amount + $wday;
            $unit = 'd';
        }
        if ($unit eq 'd') {
            $date -= $sec + 60*$min + 3600*$hour + 24*3600*$amount;
            return time2str("%Y-%m-%d %H:%M:%S", $date);
        }
        elsif ($unit eq 'y') {
            return sprintf("%4d-01-01 00:00:00", $year+1900-$amount);
        }
        elsif ($unit eq 'm') {
            $month -= $amount;
            while ($month<0) { $year--; $month += 12; }
            return sprintf("%4d-%02d-01 00:00:00", $year+1900, $month+1);
        }
        elsif ($unit eq 'h') {
            # Special case 0h for 'beginning of this hour'
            if ($amount == 0) {
                $date -= $sec + 60*$min;
            } else {
                $date -= 3600*$amount;
            }
            return time2str("%Y-%m-%d %H:%M:%S", $date);
        }
        return undef;                      # should not happen due to regexp at top
    }
    my $date = str2time($str);
    if (!defined($date)) {
        ThrowUserError("illegal_date", { date => $str });
    }
    return time2str("%Y-%m-%d %H:%M:%S", $date);
}

sub build_subselect {
    my ($outer, $inner, $table, $cond) = @_;
    my $q = "SELECT $inner FROM $table WHERE $cond";
    #return "$outer IN ($q)";
    my $dbh = Bugzilla->dbh;
    my $list = $dbh->selectcol_arrayref($q);
    return "1=2" unless @$list; # Could use boolean type on dbs which support it
    return $dbh->sql_in($outer, $list);}

sub GetByWordList {
    my ($field, $strs) = (@_);
    my @list;
    my $dbh = Bugzilla->dbh;
    return [] unless defined $strs;

    foreach my $w (split(/[\s,]+/, $strs)) {
        my $word = $w;
        if ($word ne "") {
            $word =~ tr/A-Z/a-z/;
            $word = $dbh->quote('(^|[^a-z0-9])' . quotemeta($word) . '($|[^a-z0-9])');
            trick_taint($word);
            push(@list, $dbh->sql_regexp($field, $word));
        }
    }

    return \@list;
}

# Support for "any/all/nowordssubstr" comparison type ("words as substrings")
sub GetByWordListSubstr {
    my ($field, $strs) = (@_);
    my @list;
    my $dbh = Bugzilla->dbh;
    my $sql_word;

    foreach my $word (split(/[\s,]+/, $strs)) {
        if ($word ne "") {
            $sql_word = $dbh->quote($word);
            trick_taint($sql_word);
            push(@list, $dbh->sql_iposition($sql_word, $field) . " > 0");
        }
    }

    return \@list;
}

sub getSQL {
    my $self = shift;
    return $self->{'sql'};
}

sub search_description {
    my ($self, $params) = @_;
    my $desc = $self->{'search_description'} ||= [];
    if ($params) {
        push(@$desc, $params);
    }
    return $self->{'search_description'};
}

sub pronoun {
    my ($noun, $user) = (@_);
    if ($noun eq "%user%") {
        if ($user->id) {
            return $user->id;
        } else {
            ThrowUserError('login_required_for_pronoun');
        }
    }
    if ($noun eq "%reporter%") {
        return "bugs.reporter";
    }
    if ($noun eq "%assignee%") {
        return "bugs.assigned_to";
    }
    if ($noun eq "%qacontact%") {
        return "bugs.qa_contact";
    }
    return 0;
}

# Validate that the query type is one we can deal with
sub IsValidQueryType
{
    my ($queryType) = @_;
    if (grep { $_ eq $queryType } qw(specific advanced)) {
        return 1;
    }
    return 0;
}

# Splits out "asc|desc" from a sort order item.
sub split_order_term {
    my $fragment = shift;
    $fragment =~ /^(.+?)(?:\s+(ASC|DESC))?$/i;
    my ($column_name, $direction) = (lc($1), uc($2 || ''));
    return wantarray ? ($column_name, $direction) : $column_name;
}

# Used to translate old SQL fragments from buglist.cgi's "order" argument
# into our modern field IDs.
sub translate_old_column {
    my ($column) = @_;
    # All old SQL fragments have a period in them somewhere.
    return $column if $column !~ /\./;

    if ($column =~ /\bAS\s+(\w+)$/i) {
        return $1;
    }
    # product, component, classification, assigned_to, qa_contact, reporter
    elsif ($column =~ /map_(\w+?)s?\.(login_)?name/i) {
        return $1;
    }
    
    # If it doesn't match the regexps above, check to see if the old 
    # SQL fragment matches the SQL of an existing column
    foreach my $key (%{ COLUMNS() }) {
        next unless exists COLUMNS->{$key}->{name};
        return $key if COLUMNS->{$key}->{name} eq $column;
    }

    return $column;
}

#####################################################################
# Search Functions
#####################################################################

sub _invalid_combination {
    my ($self, $args) = @_;
    my ($field, $operator) = @$args{qw(field operator)};
    ThrowUserError('search_field_operator_invalid',
                   { field => $field, operator => $operator });
}

sub _contact_pronoun {
    my ($self, $args) = @_;
    my ($value, $quoted) = @$args{qw(value quoted)};
    my $user = $self->{'user'};
    
    if ($value =~ /^\%group/) {
        $self->_contact_exact_group($args);
    }
    elsif ($value =~ /^(%\w+%)$/) {
        $args->{value} = pronoun($1, $user);
        $args->{quoted} = $args->{value};
    }
}

sub _contact_exact_group {
    my ($self, $args) = @_;
    my ($value, $operator, $field, $chart_id, $joins) =
        @$args{qw(value operator field chart_id joins)};
    my $dbh = Bugzilla->dbh;
    
    $value =~ /\%group\.([^%]+)%/;
    my $group = Bugzilla::Group->check($1);
    $group->check_members_are_visible();
    my $group_ids = Bugzilla::Group->flatten_group_membership($group->id);
    my $table = "user_group_map_$chart_id";
    my $join_sql =
        "LEFT JOIN user_group_map AS $table"
        .     " ON $table.user_id = bugs.$field"
        .        " AND " . $dbh->sql_in("$table.group_id", $group_ids)
        .        " AND $table.isbless = 0";
    push(@$joins, $join_sql);
    if ($operator =~ /^not/) {
        $args->{term} = "$table.group_id IS NULL";
    }
    else {
        $args->{term} = "$table.group_id IS NOT NULL";
    }
}

sub _contact_nonchanged {
    my ($self, $args) = @_;
    my $field = $args->{field};
    
    $args->{full_field} = "profiles.login_name";
    $self->_do_operator_function($args);
    my $term = $args->{term};
    $args->{term} = "bugs.$field IN (SELECT userid FROM profiles WHERE $term)";
}

sub _qa_contact_nonchanged {
    my ($self, $args) = @_;
    my $joins = $args->{joins};
    
    push(@$joins, "LEFT JOIN profiles AS map_qa_contact " .
                         "ON bugs.qa_contact = map_qa_contact.userid");
    $args->{full_field} = "COALESCE(map_qa_contact.login_name,'')";
}

sub _cc_pronoun {
    my ($self, $args) = @_;
    my ($full_field, $value) = @$args{qw(full_field value)};
    my $user = $self->{'user'};

    if ($value =~ /\%group/) {
        return $self->_cc_exact_group($args);
    }
    elsif ($value =~ /^(%\w+%)$/) {
        $args->{value} = pronoun($1, $user);
        $args->{quoted} = $args->{value};
        $args->{full_field} = "profiles.userid";
    }
}

sub _cc_exact_group {
    my ($self, $args) = @_;
    my ($chart_id, $sequence, $joins, $operator, $value) =
        @$args{qw(chart_id sequence joins operator value)};
    my $user = $self->{'user'};
    my $dbh = Bugzilla->dbh;
    
    $value =~ m/%group\.([^%]+)%/;
    my $group = Bugzilla::Group->check($1);
    $group->check_members_are_visible();
    my $all_groups = Bugzilla::Group->flatten_group_membership($group->id);

    # This is for the email1, email2, email3 fields from query.cgi.
    if ($chart_id eq "") {
        $chart_id = "CC$$sequence";
        $args->{sequence}++;
    }
    
    my $group_table = "user_group_map_$chart_id";
    my $cc_table = "cc_$chart_id";
    push(@$joins, "LEFT JOIN cc AS $cc_table " .
                         "ON bugs.bug_id = $cc_table.bug_id");
    my $join_sql =
        "LEFT JOIN user_group_map AS $group_table"
        .     " ON $group_table.user_id = $cc_table.who"
        .        " AND " . $dbh->sql_in("$group_table.group_id", $all_groups)
        .        " AND $group_table.isbless = 0 ";
    push(@$joins, $join_sql);
    if ($operator =~ /^not/) {
        $args->{term} = "$group_table.group_id IS NULL";
    }
    else {
        $args->{term} = "$group_table.group_id IS NOT NULL";
    }
}

sub _cc_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $sequence, $field, $full_field, $operator, $joins, $value) =
        @$args{qw(chart_id sequence field full_field operator joins value)};

    # This is for the email1, email2, email3 fields from query.cgi.
    if ($chart_id eq "") {
        $chart_id = "CC$sequence";
        $args->{sequence}++;
    }
    
    # $full_field might have been changed by one of the cc_pronoun
    # functions, in which case we leave it alone.
    if ($full_field eq 'bugs.cc') {
        $args->{full_field} = "profiles.login_name";
    }
    
    $self->_do_operator_function($args);
    
    my $term = $args->{term};
    my $table = "cc_$chart_id";
    my $join_sql =
        "LEFT JOIN cc AS $table"
        .     " ON bugs.bug_id = $table.bug_id"
         .       " AND $table.who IN (SELECT userid FROM profiles WHERE $term)";
    push(@$joins, $join_sql);
    
    $args->{term} = "$table.who IS NOT NULL";
}

# XXX This duplicates having Commenter as a search field.
sub _long_desc_changedby {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $value) = @$args{qw(chart_id joins value)};
    
    my $table = "longdescs_$chart_id";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id");
    my $user_id = login_to_id($value, THROW_ERROR);
    $args->{term} = "$table.who = $user_id";
}

sub _long_desc_changedbefore_after {
    my ($self, $args) = @_;
    my ($chart_id, $operator, $value, $joins) =
        @$args{qw(chart_id operator value joins)};
    my $dbh = Bugzilla->dbh;
    
    my $sql_operator = ($operator =~ /before/) ? '<=' : '>=';
    my $table = "longdescs_$chart_id";
    my $sql_date = $dbh->quote(SqlifyDate($value));
    my $join_sql =
        "LEFT JOIN longdescs AS $table "
        .        " ON $table.bug_id = bugs.bug_id"
        .           " AND $table.bug_when $sql_operator $sql_date";
    push(@$joins, $join_sql);
    $args->{term} = "$table.bug_when IS NOT NULL";
}

sub _content_matches {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $fields, $operator, $value) =
        @$args{qw(chart_id joins fields operator value)};
    my $dbh = Bugzilla->dbh;
    
    # "content" is an alias for columns containing text for which we
    # can search a full-text index and retrieve results by relevance, 
    # currently just bug comments (and summaries to some degree).
    # There's only one way to search a full-text index, so we only
    # accept the "matches" operator, which is specific to full-text
    # index searches.

    # Add the fulltext table to the query so we can search on it.
    my $table = "bugs_fulltext_$chart_id";
    my $comments_col = "comments";
    $comments_col = "comments_noprivate" unless $self->{'user'}->is_insider;
    push(@$joins, "LEFT JOIN bugs_fulltext AS $table " .
                         "ON bugs.bug_id = $table.bug_id");
    
    # Create search terms to add to the SELECT and WHERE clauses.
    my ($term1, $rterm1) =
        $dbh->sql_fulltext_search("$table.$comments_col", $value, 1);
    my ($term2, $rterm2) =
        $dbh->sql_fulltext_search("$table.short_desc", $value, 2);
    $rterm1 = $term1 if !$rterm1;
    $rterm2 = $term2 if !$rterm2;

    # The term to use in the WHERE clause.
    my $term = "$term1 > 0 OR $term2 > 0";
    if ($operator =~ /not/i) {
        $term = "NOT($term)";
    }
    $args->{term} = $term;
    
    # In order to sort by relevance (in case the user requests it),
    # we SELECT the relevance value so we can add it to the ORDER BY
    # clause. Every time a new fulltext chart isadded, this adds more 
    # terms to the relevance sql.
    #
    # We build the relevance SQL by modifying the COLUMNS list directly,
    # which is kind of a hack but works.
    my $current = COLUMNS->{'relevance'}->{name};
    $current = $current ? "$current + " : '';
    # For NOT searches, we just add 0 to the relevance.
    my $select_term = $operator =~ /not/ ? 0 : "($current$rterm1 + $rterm2)";
    COLUMNS->{'relevance'}->{name} = $select_term;
}

sub _timestamp_translate {
    my ($self, $args) = @_;
    my $value = $args->{value};
    my $dbh = Bugzilla->dbh;

    return if $value !~ /^[\+\-]?\d+[hdwmy]$/i;
    
    $args->{value}  = SqlifyDate($value);
    $args->{quoted} = $dbh->quote($args->{value});
}

# XXX This should probably be merged with cc_pronoun.
sub _commenter_pronoun {
    my ($self, $args) = @_;
    my $value = $args->{value};
    my $user = $self->{'user'};

    if ($value =~ /^(%\w+%)$/) {
        $args->{value} = pronoun($1, $user);
        $args->{quoted} = $args->{value};
        $args->{full_field} = "profiles.userid";
    }
}

sub _commenter {
    my ($self, $args) = @_;
    my ($chart_id, $sequence, $joins, $field, $full_field, $operator) =
        @$args{qw(chart_id sequence joins field full_field operator)};

    if ($chart_id eq "") {
        $chart_id = "LD$sequence";
        $args->{sequence}++;
    }
    my $table = "longdescs_$chart_id";
    
    my $extra = $self->{'user'}->is_insider ? "" : "AND $table.isprivate = 0";
    # commenter_pronoun could have changed $full_field to something else,
    # so we only set this if commenter_pronoun hasn't set it.
    if ($full_field eq 'bugs.commenter') {
        $args->{full_field} = "profiles.login_name";
    }
    $self->_do_operator_function($args);
    my $term = $args->{term};
    my $join_sql =
        "LEFT JOIN longdescs AS $table"
        . " ON $table.bug_id = bugs.bug_id $extra"
        .    " AND $table.who IN (SELECT userid FROM profiles WHERE $term)";
    push(@$joins, $join_sql);
    $args->{term} = "$table.who IS NOT NULL";
}

sub _long_desc {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    my $table = "longdescs_$chart_id";
    my $extra = $self->{'user'}->is_insider ? "" : "AND $table.isprivate = 0";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id $extra");
    $args->{full_field} = "$table.thetext";
}

sub _longdescs_isprivate {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    my $table = "longdescs_$chart_id";
    my $extra = $self->{'user'}->is_insider ? "" : "AND $table.isprivate = 0";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id $extra");
    $args->{full_field} = "$table.isprivate";
}

sub _work_time_changedby {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $value) = @$args{qw(chart_id joins value)};
    
    my $table = "longdescs_$chart_id";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id");
    my $user_id = login_to_id($value, THROW_ERROR);
    $args->{term} = "$table.who = $user_id AND $table.work_time != 0";
}

sub _work_time_changedbefore_after {
    my ($self, $args) = @_;
    my ($chart_id, $operator, $value, $joins) =
        @$args{qw(chart_id operator value joins)};
    my $dbh = Bugzilla->dbh;
    
    my $table = "longdescs_$chart_id";
    my $sql_operator = ($operator =~ /before/) ? '<=' : '>=';
    my $sql_date = $dbh->quote(SqlifyDate($value));
    my $join_sql =
        "LEFT JOIN longdescs AS $table"
        .     " ON $table.bug_id = bugs.bug_id"
        .        " AND $table.work_time != 0"
        .        " AND $table.bug_when $sql_operator $sql_date";
    push(@$joins, $join_sql);
    
    $args->{term} = "$table.bug_when IS NOT NULL";
}

sub _work_time {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    my $table = "longdescs_$chart_id";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id");
    $args->{full_field} = "$table.work_time";
}

sub _percentage_complete {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $operator, $value, $having, $fields) =
        @$args{qw(chart_id joins operator value having fields)};

    my $table = "longdescs_$chart_id";

    # We can't just use "percentage_complete" as the field, because
    # (a) PostgreSQL doesn't accept it in the HAVING clause
    # and (b) it wouldn't work in multiple chart rows, because it uses
    # a fixed name for the table, "ldtime".
    my $expression = COLUMNS->{percentage_complete}->{name};
    $expression =~ s/\bldtime\b/$table/g;
    $args->{full_field} = "($expression)";
    push(@$joins, "LEFT JOIN longdescs AS $table " .
                         "ON $table.bug_id = bugs.bug_id");

    # We need remaining_time in _select_columns, otherwise we can't use
    # it in the expression for creating percentage_complete.
    $self->_add_extra_column('remaining_time');

    $self->_do_operator_function($args);
    push(@$having, $args->{term});
   
    # We put something into $args->{term} so that do_search_function
    # stops processing.
    $args->{term} = "0=0";
}

sub _bug_group_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field) = @$args{qw(chart_id joins field)};
    
    my $map_table = "bug_group_map_$chart_id";
    push(@$joins,
        "LEFT JOIN bug_group_map AS $map_table " .
               "ON bugs.bug_id = $map_table.bug_id");
    
    my $groups_table = "groups_$chart_id";
    my $full_field = "$groups_table.name";
    $args->{full_field} = $full_field;
    $self->_do_operator_function($args);
    my $term = $args->{term};
    push(@$joins,
        "LEFT JOIN groups AS $groups_table " .
               "ON $groups_table.id = $map_table.group_id AND $term");
    $args->{term} = "$full_field IS NOT NULL";
}

sub _attach_data_thedata {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    my $attach_table = "attachments_$chart_id";
    my $data_table = "attachdata_$chart_id";
    my $extra = $self->{'user'}->is_insider
                ? "" : "AND $attach_table.isprivate = 0";
    push(@$joins, "LEFT JOIN attachments AS $attach_table " .
                         "ON bugs.bug_id = $attach_table.bug_id $extra");
    push(@$joins, "LEFT JOIN attach_data AS $data_table " .
                       "ON $data_table.id = $attach_table.attach_id");
    $args->{full_field} = "$data_table.thedata";
}

sub _attachments_submitter {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    my $attach_table = "attachment_submitter_$chart_id";
    my $extra = $self->{'user'}->is_insider
                ? "" : "AND $attach_table.isprivate = 0";
    push(@$joins, "LEFT JOIN attachments AS $attach_table " .
                         "ON bugs.bug_id = $attach_table.bug_id $extra");
    
    my $map_table = "map_attachment_submitter_$chart_id";
    push(@$joins, "LEFT JOIN profiles AS $map_table " .
                         "ON $attach_table.submitter_id = $map_table.userid");
    $args->{full_field} = "$map_table.login_name";
}

sub _attachments {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator, $value) =
        @$args{qw(chart_id joins field operator value)};
    my $dbh = Bugzilla->dbh;
    
    my $table = "attachments_$chart_id";
    my $extra = $self->{'user'}->is_insider ? "" : "AND $table.isprivate = 0";
    push(@$joins, "LEFT JOIN attachments AS $table " .
                         "ON bugs.bug_id = $table.bug_id $extra");
    
    $field =~ /^attachments\.(.+)$/;
    my $attach_field = $1;
    
    $args->{full_field} = "$table.$attach_field";
}

sub _join_flag_tables {
    my ($self, $args) = @_;
    my ($joins, $chart_id) = @$args{qw(joins chart_id)};
    
    my $attachments = "attachments_$chart_id";
    my $extra = $self->{'user'}->is_insider
                ? "" : "AND $attachments.isprivate = 0";
    push(@$joins, "LEFT JOIN attachments AS $attachments " .
                         "ON bugs.bug_id = $attachments.bug_id $extra");
    my $flags = "flags_$chart_id";
    # We join both the bugs and the attachments table in separately,
    # and then the join code will later combine the terms.
    push(@$joins, "LEFT JOIN flags AS $flags " . 
                         "ON bugs.bug_id = $flags.bug_id ");
    push(@$joins, "LEFT JOIN flags AS $flags " .
                         "ON $flags.attach_id = $attachments.attach_id " .
                             "OR $flags.attach_id IS NULL");
}

sub _flagtypes_name {
    my ($self, $args) = @_;
    my ($chart_id, $operator, $joins, $field, $having) = 
        @$args{qw(chart_id operator joins field having)};
    my $dbh = Bugzilla->dbh;
    
    # Matches bugs by flag name/status.
    # Note that--for the purposes of querying--a flag comprises
    # its name plus its status (i.e. a flag named "review" 
    # with a status of "+" can be found by searching for "review+").
    
    # Don't do anything if this condition is about changes to flags,
    # as the generic change condition processors can handle those.
    return if $operator =~ /^changed/;
    
    # Add the flags and flagtypes tables to the query.  We do 
    # a left join here so bugs without any flags still match 
    # negative conditions (f.e. "flag isn't review+").
    $self->_join_flag_tables($args);
    my $flags = "flags_$chart_id";
    my $flagtypes = "flagtypes_$chart_id";
    push(@$joins, "LEFT JOIN flagtypes AS $flagtypes " . 
                         "ON $flags.type_id = $flagtypes.id");
    
    # Generate the condition by running the operator-specific
    # function. Afterwards the condition resides in the $args->{term}
    # variable.
    my $full_field = $dbh->sql_string_concat("$flagtypes.name",
                                             "$flags.status");
    $args->{full_field} = $full_field;
    $self->_do_operator_function($args);
    my $term = $args->{term};
    
    # If this is a negative condition (f.e. flag isn't "review+"),
    # we only want bugs where all flags match the condition, not 
    # those where any flag matches, which needs special magic.
    # Instead of adding the condition to the WHERE clause, we select
    # the number of flags matching the condition and the total number
    # of flags on each bug, then compare them in a HAVING clause.
    # If the numbers are the same, all flags match the condition,
    # so this bug should be included.
    if ($operator =~ /^not/) {
       push(@$having,
            "SUM(CASE WHEN $full_field IS NOT NULL THEN 1 ELSE 0 END) = " .
            "SUM(CASE WHEN $term THEN 1 ELSE 0 END)");
       $args->{term} = "0=0";
    }
}

# XXX These two functions can probably be joined (requestees and setters).
sub _requestees_login_name {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    $self->_join_flag_tables($args);
    my $flags = "flags_$chart_id";
    my $map_table = "map_flag_requestees_$chart_id";
    push(@$joins, "LEFT JOIN profiles AS $map_table " .
                         "ON $flags.requestee_id = $map_table.userid");

    $args->{full_field} = "$map_table.login_name";
}

sub _setters_login_name {
    my ($self, $args) = @_;
    my ($chart_id, $joins) = @$args{qw(chart_id joins)};
    
    $self->_join_flag_tables($args);
    my $flags = "flags_$chart_id";
    my $map_table = "map_flag_setters_$chart_id";
    push(@$joins, "LEFT JOIN profiles AS $map_table " .
                         "ON $flags.setter_id = $map_table.userid");

    $args->{full_field} = "$map_table.login_name";
}

sub _days_elapsed {
    my ($self, $args) = @_;
    my $dbh = Bugzilla->dbh;
    
    $args->{full_field} = "(" . $dbh->sql_to_days('NOW()') . " - " .
                                $dbh->sql_to_days('bugs.delta_ts') . ")";
}

sub _component_nonchanged {
    my ($self, $args) = @_;
    
    $args->{full_field} = "components.name";
    $self->_do_operator_function($args);
    my $term = $args->{term};
    $args->{term} = build_subselect("bugs.component_id",
        "components.id", "components", $args->{term});
}

sub _product_nonchanged {
    my ($self, $args) = @_;
    
    # Generate the restriction condition
    $args->{full_field} = "products.name";
    $self->_do_operator_function($args);
    my $term = $args->{term};
    $args->{term} = build_subselect("bugs.product_id",
        "products.id", "products", $term);
}

sub _classification_nonchanged {
    my ($self, $args) = @_;
    my $joins = $args->{joins};
    
    # Generate the restriction condition
    push(@$joins, "INNER JOIN products AS map_product " .
                          "ON bugs.product_id = map_product.id");
    $args->{full_field} = "classifications.name";
    $self->_do_operator_function($args);
    my $term = $args->{term};
    $args->{term} = build_subselect("map_product.classification_id",
        "classifications.id", "classifications", $term);
}

sub _keywords_exact {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $value, $operator) =
        @$args{qw(chart_id joins value operator)};
    my $dbh = Bugzilla->dbh;
    
    my @keyword_ids;
    foreach my $value (split(/[\s,]+/, $value)) {
        next if $value eq '';
        my $keyword = Bugzilla::Keyword->check($value);
        push(@keyword_ids, $keyword->id);
    }
    
    # XXX We probably should instead throw an error here if there were
    # just commas in the field.
    if (!@keyword_ids) {
        $args->{term} = "0=0";
        return;
    }
    
    # This is an optimization for anywords, since we already know
    # the keyword id from having checked it above.
    if ($operator eq 'anywords') {
        my $table = "keywords_$chart_id";
        $args->{term} = $dbh->sql_in("$table.keywordid", \@keyword_ids);
        push(@$joins, "LEFT JOIN keywords AS $table"
                      .     " ON $table.bug_id = bugs.bug_id");
        return;
    }
    
    $self->_keywords_nonchanged($args);
}

sub _keywords_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $value, $operator) =
        @$args{qw(chart_id joins value operator)};

    my $k_table = "keywords_$chart_id";
    my $kd_table = "keyworddefs_$chart_id";
    
    push(@$joins, "LEFT JOIN keywords AS $k_table " .
                         "ON $k_table.bug_id = bugs.bug_id");
    push(@$joins, "LEFT JOIN keyworddefs AS $kd_table " .
                         "ON $kd_table.id = $k_table.keywordid");
    
    $args->{full_field} = "$kd_table.name";
}

# XXX This should be combined with blocked_nonchanged.
sub _dependson_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator) =
        @$args{qw(chart_id joins field operator)};
    
    my $table = "dependson_$chart_id";
    my $full_field = "$table.$field";
    $args->{full_field} = $full_field;
    $self->_do_operator_function($args);
    my $term = $args->{term};
    push(@$joins, "LEFT JOIN dependencies AS $table " .
                         "ON $table.blocked = bugs.bug_id AND ($term)");
    $args->{term} = "$full_field IS NOT NULL";
}

sub _blocked_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator) =
        @$args{qw(chart_id joins field operator)};

    my $table = "blocked_$chart_id";
    my $full_field = "$table.$field";
    $args->{full_field} = $full_field;
    $self->_do_operator_function($args);
    my $term = $args->{term};
    push(@$joins, "LEFT JOIN dependencies AS $table " .
                         "ON $table.dependson = bugs.bug_id AND ($term)");
    $args->{term} = "$full_field IS NOT NULL";
}

sub _alias_nonchanged {
    my ($self, $args) = @_;
    $args->{full_field} = "COALESCE(bugs.alias, '')";
    $self->_do_operator_function($args);
}

sub _owner_idle_time_greater_less {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $value, $operator) =
        @$args{qw(chart_id joins value operator)};
    my $dbh = Bugzilla->dbh;
    
    my $table = "idle_$chart_id";
    my $quoted = $dbh->quote(SqlifyDate($value));
    
    my $ld_table = "comment_$table";
    my $comments_join =
        "LEFT JOIN longdescs AS $ld_table"
        .     " ON $ld_table.who = bugs.assigned_to"
        .        " AND $ld_table.bug_id = bugs.bug_id"
        .        " AND $ld_table.bug_when > $quoted";
    push(@$joins, $comments_join);

    my $act_table = "activity_$table";
    my $assigned_fieldid = $self->_chart_fields->{'assigned_to'}->id;

    # XXX Why are we joining using $assignedto_fieldid here? It shouldn't
    #     matter when or if the assignee changed.
    my $activity_join =
        "LEFT JOIN bugs_activity AS $act_table"
        .     " ON ( $act_table.who = bugs.assigned_to"
        .         "  OR $act_table.fieldid = $assigned_fieldid )"
        .        " AND $act_table.bug_id = bugs.bug_id"
        .        " AND $act_table.bug_when > $quoted";
    push(@$joins, $activity_join);
    
    if ($operator =~ /greater/) {
        $args->{term} =
            "$ld_table.who IS NULL AND $act_table.who IS NULL)";
    } else {
         $args->{term} =
            "$ld_table.who IS NOT NULL OR $act_table.who IS NOT NULL";
    }
}

sub _multiselect_negative {
    my ($self, $args) = @_;
    my ($field, $operator) = @$args{qw(field operator)};

    my $table;
    if ($field eq 'keywords') {
        $table = "keywords LEFT JOIN keyworddefs"
                 . " ON keywords.keywordid = keyworddefs.id";
        $args->{full_field} = "keyworddefs.name";
    }
    else { 
        $table = "bug_$field";
        $args->{full_field} = "$table.value";
    }
    $args->{operator} = $self->_reverse_operator($operator);
    $self->_do_operator_function($args);
    my $term = $args->{term};
    $args->{term} =
        "bugs.bug_id NOT IN (SELECT bug_id FROM $table WHERE $term)";
}

sub _multiselect_multiple {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator, $value)
        = @$args{qw(chart_id joins field operator value)};
    
    my $table = "bug_$field";
    $args->{full_field} = "$table.value";
    
    my @terms;
    foreach my $word (split(/[\s,]+/, $value)) {
        $args->{value} = $word;
        $self->_do_operator_function($args);
        my $term = $args->{term};
        push(@terms, "bugs.bug_id IN (SELECT bug_id FROM $table WHERE $term)");
    }
    
    if ($operator eq 'anyexact') {
        $args->{term} = join(" OR ", @terms);
    }
    else {
        $args->{term} = join(" AND ", @terms);
    }
}

sub _multiselect_nonchanged {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator) =
        @$args{qw(chart_id joins field operator)};

    my $table = "${field}_$chart_id";
    $args->{full_field} = "$table.value";
    push(@$joins, "LEFT JOIN bug_$field AS $table " .
                         "ON $table.bug_id = bugs.bug_id ");
}

sub _simple_operator {
    my ($self, $args) = @_;
    my ($full_field, $quoted, $operator) =
        @$args{qw(full_field quoted operator)};
    my $sql_operator = SIMPLE_OPERATORS->{$operator};
    $args->{term} = "$full_field $sql_operator $quoted";
}

sub _casesubstring {
    my ($self, $args) = @_;
    my ($full_field, $quoted) = @$args{qw(full_field quoted)};
    my $dbh = Bugzilla->dbh;
    
    $args->{term} = $dbh->sql_position($quoted, $full_field) . " > 0";
}

sub _substring {
    my ($self, $args) = @_;
    my ($full_field, $quoted) = @$args{qw(full_field quoted)};
    my $dbh = Bugzilla->dbh;
    
    # XXX This should probably be changed to just use LIKE
    $args->{term} = $dbh->sql_iposition($quoted, $full_field) . " > 0";
}

sub _notsubstring {
    my ($self, $args) = @_;
    my ($full_field, $quoted) = @$args{qw(full_field quoted)};
    my $dbh = Bugzilla->dbh;
    
    # XXX This should probably be changed to just use NOT LIKE
    $args->{term} = $dbh->sql_iposition($quoted, $full_field) . " = 0";
}

sub _regexp {
    my ($self, $args) = @_;
    my ($full_field, $quoted) = @$args{qw(full_field quoted)};
    my $dbh = Bugzilla->dbh;
    
    $args->{term} = $dbh->sql_regexp($full_field, $quoted);
}

sub _notregexp {
    my ($self, $args) = @_;
    my ($full_field, $quoted) = @$args{qw(full_field quoted)};
    my $dbh = Bugzilla->dbh;
    
    $args->{term} = $dbh->sql_not_regexp($full_field, $quoted);
}

sub _anyexact {
    my ($self, $args) = @_;
    my ($field, $value, $full_field) = @$args{qw(field value full_field)};
    my $dbh = Bugzilla->dbh;
    
    my @list;
    foreach my $word (split(/,/, $value)) {
        $word = trim($word);
        if ($word eq "---" && $field eq 'resolution') {
            $word = "";
        }
        my $quoted_word = $dbh->quote($word);
        trick_taint($quoted_word);
        push(@list, $quoted_word);
    }
    
    if (@list) {
        $args->{term} = $dbh->sql_in($full_field, \@list);
    }
    # XXX Perhaps if it's all commas, we should just throw an error.
    else {
        $args->{term} = "0=0";
    }
}

sub _anywordsubstr {
    my ($self, $args) = @_;
    my ($full_field, $value) = @$args{qw(full_field value)};
    
    my $list = GetByWordListSubstr($full_field, $value);
    $args->{term} = join(" OR ", @$list);
}

sub _allwordssubstr {
    my ($self, $args) = @_;
    my ($full_field, $value) = @$args{qw(full_field value)};
    
    my $list = GetByWordListSubstr($full_field, $value);
    $args->{term} = join(" AND ", @$list);
}

sub _nowordssubstr {
    my ($self, $args) = @_;
    $self->_anywordsubstr($args);
    my $term = $args->{term};
    $args->{term} = "NOT($term)";
}

sub _anywords {
    my ($self, $args) = @_;
    my ($full_field, $value) = @$args{qw(full_field value)};
    
    my $list = GetByWordList($full_field, $value);
    $args->{term} = join(" OR ", @$list);
}

sub _allwords {
    my ($self, $args) = @_;
    my ($full_field, $value) = @$args{qw(full_field value)};
    
    my $list = GetByWordList($full_field, $value);
    $args->{term} = join(" AND ", @$list);
}

sub _nowords {
    my ($self, $args) = @_;
    $self->_anywords($args);
    my $term = $args->{term};
    $args->{term} = "NOT($term)";
}

sub _changedbefore_changedafter {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator, $value) =
        @$args{qw(chart_id joins field operator value)};
    my $dbh = Bugzilla->dbh;
    
    my $sql_operator = ($operator =~ /before/) ? '<=' : '>=';
    my $field_object = $self->_chart_fields->{$field}
        || ThrowCodeError("invalid_field_name", { field => $field });
    my $field_id = $field_object->id;
    # Charts on changed* fields need to be field-specific. Otherwise,
    # OR chart rows make no sense if they contain multiple fields.
    my $table = "act_${field_id}_$chart_id";

    my $sql_date = $dbh->quote(SqlifyDate($value));
    push(@$joins,
        "LEFT JOIN bugs_activity AS $table"
        .     " ON $table.bug_id = bugs.bug_id"
        .         " AND $table.fieldid = $field_id"
        .         " AND $table.bug_when $sql_operator $sql_date");
    $args->{term} = "$table.bug_when IS NOT NULL";
}

sub _changedfrom_changedto {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator, $quoted) =
        @$args{qw(chart_id joins field operator quoted)};
    
    my $column = ($operator =~ /from/) ? 'removed' : 'added';
    my $field_object = $self->_chart_fields->{$field}
        || ThrowCodeError("invalid_field_name", { field => $field });
    my $field_id = $field_object->id;
    my $table = "act_${field_id}_$chart_id";
    push(@$joins,
        "LEFT JOIN bugs_activity AS $table"
        .     " ON $table.bug_id = bugs.bug_id"
        .        " AND $table.fieldid = $field_id"
        .        " AND $table.$column = $quoted");
    $args->{term} = "$table.bug_when IS NOT NULL";
}

sub _changedby {
    my ($self, $args) = @_;
    my ($chart_id, $joins, $field, $operator, $value) =
        @$args{qw(chart_id joins field operator value)};
    
    my $field_object = $self->_chart_fields->{$field}
        || ThrowCodeError("invalid_field_name", { field => $field });
    my $field_id = $field_object->id;
    my $table = "act_${field_id}_$chart_id";
    my $user_id  = login_to_id($value, THROW_ERROR);
    push(@$joins,
        "LEFT JOIN bugs_activity AS $table"
        .     " ON $table.bug_id = bugs.bug_id"
        .        " AND $table.fieldid = $field_id"
        .        " AND $table.who = $user_id");
    $args->{term} = "$table.bug_when IS NOT NULL";
}

1;
