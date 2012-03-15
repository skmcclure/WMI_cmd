#!/usr/bin/perl
#
# Create ejs interfaces for structures in a C header file
#

use File::Basename;
use Data::Dumper;

#
# Generate parse tree for header file
#

my $file = shift;
require smb_interfaces;
my $parser = new smb_interfaces;
$header = $parser->parse($file);

#
# Make second pass over tree to make it easier to process.
#

sub flatten_structs($) {
  my $obj = shift;
  my $s = { %$obj };

  # Map NAME, STRUCT_NAME and UNION_NAME elements into a more likeable
  # property.

  if (defined($obj->{STRUCT_NAME}) or defined($obj->{UNION_NAME})) {

    $s->{TYPE_DEFINED} = defined($obj->{STRUCT_NAME}) ? $obj->{STRUCT_NAME} 
      : $obj->{UNION_NAME};

    delete $s->{STRUCT_NAME};
    delete $s->{UNION_NAME};
  }

  # Create a new list of structure fields with flattened names

  foreach my $elt (@{$obj->{DATA}}) {
    foreach my $name (@{$elt->{NAME}}) {
      my $new_elt = { %$elt };
      $new_elt->{NAME} = $name;
#      $new_elt->{PARENT} = $s;
      push(@{$s->{FIELDS}}, flatten_structs($new_elt));
    }
  }

  delete $s->{DATA};

  return $s;
}

@newheader = map { flatten_structs($_) } @{$header};

#
# Generate implementation
#

my $basename = basename($file, ".h");
stat "libcli/gen_raw" || mkdir("libcli/gen_raw") || die("mkdir");

open(FILE, ">libcli/gen_raw/ejs_${basename}.c");

print FILE "/* EJS wrapper functions auto-generated by build_smb_interfaces.pl */\n\n";

print FILE "#include \"includes.h\"\n";
print FILE "#include \"scripting/ejs/smbcalls.h\"\n";
print FILE "#include \"lib/appweb/ejs/ejs.h\"\n";
print FILE "#include \"scripting/ejs/ejsrpc.h\"\n"; # TODO: remove this
print FILE "\n";

sub transfer_element($$$) {
  my $dir = shift;
  my $prefix = shift;
  my $elt = shift;

  $type = $elt->{TYPE};
  $type =~ s/_t$//;

  print FILE "\tNDR_CHECK(ejs_${dir}_$type(ejs, v, \"$prefix.$elt->{NAME}\"));\n";
}

sub transfer_struct($$) {
  my $dir = shift;
  my $struct = shift;

  foreach my $field (@{$struct->{FIELDS}}) {
    next if $dir eq "pull" and $field->{NAME} eq "out";
    next if $dir eq "push" and $field->{NAME} eq "in";

    if ($field->{TYPE} eq "struct") {
      foreach $subfield (@{$field->{FIELDS}}) {
	transfer_element($dir, $field->{NAME}, $subfield);
      }
    } else {
      transfer_element($dir, $struct->{NAME}, $field);
    }
  }
}

# Top level call functions

foreach my $s (@newheader) {

  if ($s->{TYPE} eq "struct") {

    # Push/pull top level struct

    print FILE "NTSTATUS ejs_pull_$s->{TYPE_DEFINED}(struct ejs_rpc *ejs, struct MprVar *v, struct $s->{TYPE_DEFINED} *r)\n";
    print FILE "{\n";

    transfer_struct("pull", $s);

    print FILE "\n\treturn NT_STATUS_OK;\n";
    print FILE "}\n\n";

    print FILE "NTSTATUS ejs_push_$s->{TYPE_DEFINED}(struct ejs_rpc *ejs, struct MprVar *v, const struct $s->{TYPE_DEFINED} *r)\n";
    print FILE "{\n";

    transfer_struct("push", $s);

    print FILE "\n\treturn NT_STATUS_OK;\n";
    print FILE "}\n\n";

    # Function call

    print FILE "static int ejs_$s->{TYPE_DEFINED}(int eid, int argc, struct MprVar **argv)\n";
    print FILE "{\n";
    print FILE "\treturn ejs_raw_call(eid, argc, argv, (ejs_pull_function_t)ejs_pull_$s->{TYPE_DEFINED}, (ejs_push_function_t)ejs_push_$s->{TYPE_DEFINED});\n";
    print FILE "}\n\n";

  } else {

    # Top level union

    foreach my $arm (@{$s->{FIELDS}}) {

      # Push/pull union arm

      print FILE "NTSTATUS ejs_pull_$s->{TYPE_DEFINED}_$arm->{NAME}(struct ejs_rpc *ejs, struct MprVar *v, union $s->{TYPE_DEFINED} *r)\n";
      print FILE "{\n";

      transfer_struct("pull", $arm);

      print FILE "\n\treturn NT_STATUS_OK;\n";
      print FILE "}\n\n";

      print FILE "NTSTATUS ejs_push_$s->{TYPE_DEFINED}_$arm->{NAME}(struct ejs_rpc *ejs, struct MprVar *v, const union $s->{TYPE_DEFINED} *r)\n";
      print FILE "{\n";

      transfer_struct("push", $arm);

      print FILE "\n\treturn NT_STATUS_OK;\n";
      print FILE "}\n\n";

    }
  }
}

close(FILE);