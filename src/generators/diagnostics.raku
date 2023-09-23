
my $time = DateTime.now(
    formatter=>{
        sprintf "%04d/%02d/%02d %02d:%02d:%02d", .year, .month, .day, .hour, .minute, .second given $^self
    });

my $file = 'diagnostics.def'.IO.slurp;

chdir '..';

my @groups = ();

my $line = 1;
my $col = 1;


$file ~~ s:g/\/\/.*?$$/\n/;

parse-group;

sub skip-whitespace {
    loop {
        if $file ~~ /^\n/ {
            $line += 1;
            $col = 1;
            $file = $file.substr(1, *);
        } else {
            my $m = $file ~~ m/^\h+/;
            last unless $m;
            $col += $m.chars;
            $file = $file.substr($m.chars, *);
        }
    }
}

sub expect-and-eat($re) {
    skip-whitespace;
    my $dinner = $file ~~ $re or die "expected {$re.raku} at $line:$col";
    my $out = $file.substr(0, $dinner.chars);
    $file = $file.substr($dinner.chars, *);
    $col += $out.chars;
    skip-whitespace;
    return $out;
}

sub parse-group {
    skip-whitespace;
    while $file {
        expect-and-eat /^group/;
        my $id = expect-and-eat /^\w+/;
        expect-and-eat /^'{'/;
        my $group = (group => [$id]);
        my @data := $group.value;

        until $file ~~ /^'}'/ or not $file {
            my $type = expect-and-eat /^[diagnostic || group]/;
            given $type {
                when $type eq 'group' {@data.push: parse-group}
                when $type eq 'diagnostic' {@data.push: parse-diag}
            }
        }
        expect-and-eat /^'}'/;
        @groups.push: $group;
    }
}

sub parse-diag {
    my $id = expect-and-eat /^\w+/;
    expect-and-eat /^'{'/;  
    my $diag = (diagnostic => [$id]);
    my @data := $diag.value;

    while 1 {
        last if $file ~~ /^'}'/;
        my $key = expect-and-eat /^[ type<|w> || message<|w> ]/;
        given $key {
            when 'type' {
                expect-and-eat /^':'/;
                my $val = expect-and-eat /^[ error<|w> || warning<|w> ]/;
                @data.push: (type => $val);
                expect-and-eat /^';'/;
            }
            when 'message' {
                expect-and-eat /^':'/;
                expect-and-eat /^'{'/;
                my @messages = ();
                until $file ~~ /^\s*'}'/ {
                    my $key = expect-and-eat /^\w+/;
                    expect-and-eat /^':'/;
                    my $val = expect-and-eat /^'"'.*?<!before \\>'"'/;
                    $val ~~ s/'"'(.*?)<!before \\>'"'/$0/;
                    @messages.push: ($key => $val);
                    expect-and-eat /^';'/;
                }
                @data.push: (messages => @messages);
                expect-and-eat /^'}'/;
            }
        }
    }
    expect-and-eat /^'}'/;
    $diag
}

my $count = 1;
my $header = 
    "/* {$time.Instant.to-posix[0].Int} [$time]\n" ~
    "    generated by diagnostics.raku from diagnostics.def\n" ~
    "*/\n";
my $funcs = 
    "namespace amu \{\n" ~
    "namespace diagnostic \{\n" ~
    "language lang;\n";
my $funcs-header = 
	"namespace amu \{\n" ~
	"struct MessageSender;\n" ~ # need to forward include this
	"namespace diagnostic \{\n";
my $enum = "enum kind \{\n\tOK=0,\n";
my $strs = "const global String strings[] = \{\n";

my $enum-prefix = "";

construct-diagnostics;

sub construct-diagnostics {
    for @groups -> $group {
        construct-group($group.value);
    }
}

sub construct-group($group) {

	($funcs, $funcs-header) >>~=>> "namespace {$group[0]} \{\n"; 
    $enum-prefix ~= "{$group[0]}_";
    for $group[1..*] -> $child {
        when $child.key eq 'group' {construct-group($child.value)}
        when $child.key eq 'diagnostic' {construct-diag($child.value)}
    }
    $enum-prefix = $enum-prefix.chop: $group[0].chars+1;
    ($funcs, $funcs-header) >>~=>> "\} // namespace {$group[0]}\n\n";
}

sub construct-diag(@diag) {
    $enum ~= "\t$enum-prefix" ~ "{@diag[0]} = $count,\n";
    $strs ~= "\t\"$enum-prefix" ~ "{@diag[0]}\",\n";
    
    my $type;
    my @messages;

    for @diag[1..*] -> $child {
        when $child.key eq 'type' { $type = $child.value; }
        when $child.key eq 'messages' { @messages = $child.value; }
    }

    die "type not defined for {@diag[0]}" without $type;
    die "message not defined for {@diag[0]}" without @messages;

    my @dynparts = [];
    my @locales = [];
    for @messages -> $message {
        # split message where types are found 
        my @parts = $message.value.split(/\%\w+[\:\w+]?\%|\$\d+\$/, :v).grep(/.+/);
        my @locdynparts = [];
        my @locdynpartsstrs = []; # for back ref - can definitely do this better but im tired
        my $argcount = 0;
        for @parts -> $part is rw {
            if $part.starts-with: '%' {
                my $split = $part.split(':');
                # if we have found a named arg or the dynpart has already defined a name, emplace a named thing yeah whatever 
                if $split.elems > 1 || @dynparts && @dynparts[$argcount] ~~ Pair {
                    my $name;
                    if @dynparts {
                        $name = @dynparts[$argcount].kv[1]; 
                        $split .= substr(0,*-1);
                    } else {
                        $name = $split[1].substr(0,*-1);
                    }
                    given $split[0].substr(1,*) {
                        when 'String' {
                            @locdynparts.push: 'String' => $name;
                            $part = $name;
                        }
                        when 'identifier' {
                            @locdynparts.push: 'String' => $name;
                            $part = "message::identifier($name)";
                        }
                        when 'path' {
                            @locdynparts.push: 'String' => $name;
                            $part = "message::path($name)";
                        }
                        when 'token' {
                            @locdynparts.push: 'Token*' => $name;
                            $part = $name;
                        }
                        when 'type' {
                            @locdynparts.push: 'Type*' => $name;
                            $part = $name;
                        }
                        when 'num' {
                            @locdynparts.push: 's64' => $name;
                            $part = "to_string($name)->fin"; # !Leak: Messenger needs to take DStrings so we don't cause leaks like this 
                        }
                    }
                } else {
                    given $split[0].substr(1,*-1) {
                        when 'String' {
                            @locdynparts.push: 'String';
                            $part = "arg$argcount";
                        }
                        when 'identifier' {
                            @locdynparts.push: 'String';
                            $part = "message::identifier(arg$argcount)";
                        }
                        when 'path' {
                            @locdynparts.push: 'String';
                            $part = "message::path(arg$argcount)";
                        }
                        when 'token' {
                            @locdynparts.push: 'Token*';
                            $part = "arg$argcount";
                        }
                        when 'type' {
                            @locdynparts.push: 'Type*';
                            $part = "arg$argcount";
                        }
                        when 'num' {
                            @locdynparts.push: 's64';
                            $part = "to_string(arg$argcount)->fin";
                        }
                    }
                }
                $argcount += 1;
                @locdynpartsstrs.push: $part;
            } elsif $part.starts-with: '$' {
                my $index = $part.substr(1,*-1).Int;
                die "in '{@diag[0]}': backref index $index was given, but there are only {$argcount} args so far"
                    if $index >= $argcount;
                
                $part = @locdynpartsstrs[$index];
            } else {
                $part = 'String("' ~ $part ~ '")';
            }
            
        }

        once @dynparts = @locdynparts;

        die "in '{@diag[0]}': locale '{$message.key}' uses a different amount of dynamic parts than the previous messages"
            if @dynparts.elems != @locdynparts.elems;

        die "in '{@diag[0]}': unknown locale '{$message.key}'" 
            if not $message.key eq any('en', 'jp', 'esp');
        
        @locales.push: ($message.key => @parts);
    }

    my $sig = "";
    for @dynparts.kv -> $num, $part {
        if $part ~~ Pair {
            $sig ~= ", {$part.kv[0]} {$part.kv[1]}";
        } else {
            $sig ~= ", $part arg{$num}";
        }
    }

    my @message-commands = [];

    my $argcount = 0;
    for @locales -> $locale {

        my $command = "case {$locale.key}: \{\n";
        for $locale.value -> $part {
            # when $part eq '%%' {
            #     $command ~= "arg$argcount, ";
            # }
            $command ~= "\t\t\tmessage::push(out, $part);\n";
        }
        $command ~= "\t\t} break;";
        @message-commands.push: $command;
    }
    
	my $prototype = "FORCE_INLINE global void {@diag[0]}(MessageSender sender$sig)";

	$funcs-header ~= "$prototype;\n";

    $funcs ~= 
    "$prototype \{\n" ~
    "\tDiagnostic diag = \{0\};\n" ~
    "\tdiag.code = $count;\n" ~
    "\tdiag.severity = diagnostic::$type;\n" ~
    "\tif(!sender.type) ::amu::compiler::instance.diagnostics.push(diag);\n" ~
    "\telse code::add_diagnostic(sender.code, diag);\n" ~
    "\tMessage out = message::init();\n" ~
    "\tout.kind = message::$type;\n" ~
    "\tswitch(lang) \{\n";
    for @message-commands -> $command {
        $funcs ~= "\t\t$command\n";
    }
    $funcs ~= 
    "\t}\n" ~
    "\tout = message::attach_sender(sender, out);\n" ~
    "\tmessenger::dispatch(out);\n" ~
    "}\n\n";
    $count += 1;
}


$funcs-header ~= "\} // namespace diagnostic\n\} // namespace amu";
$funcs-header = $header ~ $funcs-header;
$funcs ~= "\} // namespace diagnostic\n\} // namespace amu";
$funcs = $header ~ $funcs;
($enum, $strs) >>~=>> "\};";
my $data = $header ~ $enum ~ "\n\n" ~ $strs;
$data ~= 
"\n\nenum language \{\n" ~
"\ten,\n" ~
"\tjp,\n" ~
"\tesp,\n" ~
"};\n\n" ~
"enum severity \{\n" ~
"\terror,\n" ~
"\twarning,\n" ~
"};\n";

my $funcs-out = open "../src/data/diagnostic-impl.generated", :w or die "unable to open impl file";
my $funcs-header-out = open "../src/data/diagnostic-header.generated", :w or die "unable to open header file";
my $data-out = open "../src/data/diagnostics-data.generated", :w or die "unable to open data file";

$funcs-header-out.say($funcs-header);
$funcs-out.say($funcs);
$data-out.say($data);
