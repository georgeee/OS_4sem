#!/bin/bash
$@

#Even is we chmod this file to 4755 and chown to root:root, it'll be useless, cause it's just a script and it will be executed by /bin/bash which is set to usual 0755 =)
