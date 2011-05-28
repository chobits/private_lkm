#/bin/bash
echo "/root/module/asec/testfile 22332 DENY_READ" > /sys/kernel/security/asec/policy
echo "/bin/dd 376165 DENY_ACCESS" > /sys/kernel/security/asec/policy
echo "/root/module/char 560552 DENY_ACCESS DENY_READ" > /sys/kernel/security/asec/policy
