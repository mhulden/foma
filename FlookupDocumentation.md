# flookup #

Flookup is a separate command-line utility that allows one to pass words (strings) through transducers and automata from a command-line or through bidirectional pipes with other applications. The standard usage is in scenarios like:

```
cat mytext.txt | flookup -x mytransducer.foma
```

which would pass each line in `mytext.txt` through the transducer in the upward direction.

By default, `flookup` passes words in the upward direction (hence the name), equivalent to `apply up` or `up` in the `foma` interface. Passing the flag `-i` to `flookup` is equivalent to running `apply down`.

If several FSMs are found in the same file, `flookup` will pass a word through each of them in turn in a pipe-like fashion (see FAQ). In the upward direction (the default) the last transducer is applied up first, and the word passed to the previous transducer, etc. In the downward direction (`-i`), the word is passed through the first transducer, which provides its output to the second, etc.

The `-a` flag (for alternatives) changes this strategy. With this flag set, the transducers are tried in turn.  In the upward direction (the default) the last transducer is tried first, if that fails to provide an output, the penultimate transducer is tried, etc. until one is found that provides an output.  This is intended to be used for applications such as morphologies where the last transducer is perhaps a standard morphology, and the penultimate a guesser morphology, etc. Here, if the last transducer provides a mapping, the penultimate isn't tried, etc.  With the `-i`-flag again, the order of attempts is inverted.

The `-x` flag disables printing of the original input word.

Normally `flookup` prints out the matches separating each input word with a newline, like so:

```
input1    output1.1
input1    output1.2

input2    output2.1
...
```

The symbol for word separation can be changed with the `-w` flag. The separator between inputs and outputs (default is TAB) is controlled with the `-s` flag.

### Indexing ###

The default search strategy with `flookup` is to sort the arcs for each state in the network on either the input or output side, and then use a binary search during application.  This is memory-efficient and pretty close to optimally fast for states with less than 1,000 transitions.  If you have really dense automata, you can consider indexing some of the arcs on some of the states before applying.  This is controlled by the `-I` flag, followed by some options:

  * `-I #` indexes all states with more than `#` arcs
  * `-I f` indexes only the states that have flag diacritics (useful sometimes since those states aren't searched with a binary search)
  * `-I #k` or `-I #m` index all states from densest to sparsest, but stop when reaching memory usage of `#` kilobytes or `#` megabytes.

The options can be combined:

Example:

```
flookup -I f -I 1024k mytransducer.foma
```

would try to index all states with flag diacritics, as long as the total index wouldn't exceed 1024k.  This might result in only some of the states with flags (the densest ones) being indexed, naturally.

### Running flookup as a server ###

You can also run `flookup` as UDP server.  The behavior is controlled by the `-S` flag, and the optional arguments are the listening address `-A` and the port `-P`. This is useful if you need web page scripts to be able to access transducers easily.

If you start a server as, say:

```
flookup -S -A 127.0.0.1 mygrammar.foma &
```

you can verify that it's working with the netcat utility (`nc` on most systems).

```
echo "testword" | nc -w 1 -u 127.0.0.1 6062
```


Now, if you start the server, you can easily access the `flookup` result through, say `php`, by something like:

```
<?php
$fp  = fsockopen("udp://localhost", 6062, $errno, $errstr, 30);
fwrite($fp, $myword);
$response = fread($fp, 1024); 
fclose($fp);
print $response;
?>
```

or in `perl`, by something like:

```
#!/usr/bin/perl

use IO::Socket;
use strict;

my($sock, $word, $port, $ipaddr, $hishost, $MAXLEN, $PORTNO, $TIMEOUT);

$MAXLEN  = 2048;
$PORTNO  = 6062;
$TIMEOUT = 1;

$sock = IO::Socket::INET->new(Proto     => 'udp',
                              PeerPort  => $PORTNO,
                              PeerAddr  => 'localhost') or die "Creating socket: $!\n";

$word = 'testword';

$sock->send($word) or die "send: $!";

eval {
    local $SIG{ALRM} = sub { die "time out" };
    alarm $TIMEOUT;
    $sock->recv($word, $MAXLEN)      or die "recv: $!";
    alarm 0;
} or die "localhost timed out after $TIMEOUT seconds.\n";

print "$word\n";

```

Note that with this lookup strategy you will be sending one UDP datagram per request.  The reply is a message with all the possible output strings in it, separated by newlines.  For big batch jobs this is probably suboptimal because of network overhead, and the speeds will max out at some tens of thousands of requests per second (on localhost).  If you need to process large amounts of data, it is best to pipe through flookup, or use the API.

## cgflookup ##

`Foma` also provides a separate lookup-utility intended to be used for providing alternative morphological readings as input to constraint grammars.  The main differences between `flookup` and `cgflookup` are that:

  * `cgflookup` prints the input word surrounded by `"< ... >"` as per the CG standard.
  * `cgflookup` prints subsequent output alternatives each on a line, indented by a tab.

The utility also has a flag `-u`, which adds the feature `<*>` to those lines whose input words were uppercase-initial. Otherwise, the set of options is identical to that of `flookup`.