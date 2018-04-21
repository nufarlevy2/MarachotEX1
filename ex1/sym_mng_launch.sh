#!/bin/bash

fullPathToDataFile=$PATH_TO_DATA/$DATA_FILE #Concating 2 strings to the full path of the data file
chmod u+rwx $fullPathToDataFile #Giving all permissions to the user
chmod go-rwx $fullPathToDataFile #Reducing all permissions to group and other users
$FULL_EXE_NAME $PATH_TO_DATA/$DATA_FILE $PATTERN $BOUND & #Executing the prog with all parameters in the background!

