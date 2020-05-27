#!/bin/bash

IPCS_Q=`ipcs -q | egrep "[1-9][0-9][0-9][0-9][0-9]+" -o`

for id in $IPCS_Q
  do
    ipcrm -q $id
  done

echo "Removed all messages from queues"