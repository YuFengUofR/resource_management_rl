# resouse_management_rl

# When add parsecmgmt path into sudo path in order to
# execute 'parsecmgmt' program; 

# Open visudo by 
>> sudo visudo

# at the default path add 
secure_path='.....:\path\to\parsec\' 

# run a parsec benchmark, using command such as:
sudo parsecmgmt  -a run -p <APP> -i sim<small|middle|large>



