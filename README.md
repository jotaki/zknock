# Knocking Client

A simple wrapper script for the knock client distrbuted with [knock](http://www.zeroflux.org/projects/knock)

## Design & Features

 * zknock stores profiles in $KNOCKDIR. (by default this is $HOME/.knock)
 * profiles are encrypted with gpg AES256.
 * profiles are stored as their own filename, e.g
   * $HOME/.knock/profile1
   * $HOME/.knock/profile2
   * $HOME/.knock/profile3
   * ....
   * ....
   * ....
 * A profile config file in plaintext looks something like this:
   * `hostname="localhost"  use_udp=""  delay="-d 250"  verbose=""  ports=(12345 1234:udp 54321 )`

## Using zknock

 * Running zknock without any parameters will invoke the default profile ".default".
 * If a profile is found not to be configured an interactive guide will prompt you.
 ```bash

 $ zknock local
 /home/foo/.knock/local does not exist. would you like to configure it? [Y/n]: y
 Hostname: localhost
 Delay (in ms):
 Default to udp? [Y/n]: n
 Verbose output? [Y/n]: y
 Port   1: 12345:udp
 Port   2: 54321
 Port   3: 9000
 Port   4: 8000
 Port   5: 7000
 Port   6:

 $ zknock local
 gpg: AES256 encrypted data
 gpg: encrypted with 1 passphrase
 hitting udp 127.0.0.1:12345
 hitting tcp 127.0.0.1:54321
 hitting tcp 127.0.0.1:9000
 hitting tcp 127.0.0.1:8000
 hitting tcp 127.0.0.1:7000```

## Limitations

 * The knocking client that comes with [knock](http://www.zeroflux.org/projects/knock) does not appear to support IPv6.
