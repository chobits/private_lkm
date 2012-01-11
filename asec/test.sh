# /bin/bash

# interface with our kernel security module
policy_file=/sys/kernel/security/asec/policy
# which file to be protected
object_file=
# inode number of object file
inode_number=$(ls -i $object_file | cut -d ' ' -f 1)
# DENY_READ / DENY_WRITE /...
policy="DENY_READ"

echo "$object_file $inode_number $policy" > $policy_file
