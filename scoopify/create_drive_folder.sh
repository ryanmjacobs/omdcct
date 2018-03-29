#!/bin/bash

gdrive folder -t scoops_`date +%s` | grep Id | cut -d' ' -f2 | tee folder_id.txt
