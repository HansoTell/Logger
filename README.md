# Logger
A Async Logger for basic Logging designed for good performance

### Usage
Currently u can only have one Logger at the time wich you can Create with CREATE_LOGGER(filepath) and destroy with DESTROY_LOGGER() then you can easily log with the LOG Makros for just a 
Message an the VLOG Funktions for multiple messages in one log wich will be combined with a space. For VLOG The types, const char*, string_view, std::string, all arithemtik types
and string convertible types are supported. You can also implement a std::string toLog() const method to your class then you can put them into the VLOG funktions as well  
Log files will be automaticly changed at a file size of 5mb

### Support
Currently this projects has been only used in a unix envirmoent. It hasent 
been tested or used in any other envirment than Ubunut Linux. I cant garuntee
it working stable in any other enviroment  
Also at the moment there is no possiblitiy to Log the messages to console it can only be logged into a file

### Future Plans
Possibility for multible loggers
Sync Loggers
Logging to console
> [!IMPORTANT]
> complete Tests
> Test for failing file opening + logs with failed file opening
> Creating directorys in given path

### Disclaimer
This project got started because during the development of the Project "HTTP-Sverver". For this reason not all commits will be present 
in this repo. If you still like to see earlyer versions you have to search in the other Repo
