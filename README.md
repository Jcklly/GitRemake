# GitRemake
Recreation of GIT in C

# Synopsis
This is a multi-threaded server client project version control program with the basic capabilities of GIT.

# Commands:
## NOTE: This was created and tested in a Linux environment
- Please type `make` to create the executables.
- First run `./WTF configure localhost PORT` where PORT is PORT > 9000 && PORT < 65000.
  - localhost could be replaced with any valid IP Address, but if you are running on the same system then use localhost or 127.0.0.1
- next run `./WTFserver PORT` where PORT is the same number you entered for configure
- Open a new terminal window and run any of the following commands:
  - `./WTF create PROJECT-NAME`
    - creates a project
  - `./WTF destroy PROJECT-NAME`
    - deleted a project from server
  - `./WTF add PROJECT-NAME FILENAME`
    - Adds file to version control for project. Please enter the FULL PATH of the file.
    - Example: `./WTF add proj1 proj1/file1.txt`
  - `./WTF remove PROJECT-NAME FILENAME`
    - Removes file from version control for project. Please enter the FULL PATH of the file.
    - Example: `./WTF remove proj1 proj1/file1.txt`
  - `./WTF currentversion PROJECT-NAME FILENAME`
    - Check current version of given project
  - `./WTF history PROJECT-NAME`
    - Shows the full history of a project
  - `./WTF rollback PROJECT-NAME VERSION`
    - Rolls back the project to a previous version. Will have to perform a Update+Upgrade or a checkout after rollback to update client side
  - `./WTF checkout PROJECT-NAME`
    - Copies current project on server to client
  - `./WTF update PROJECT-NAME`
    - Set's up the changes between the server and client's version of the project. Must be performed BEFORE upgrade
  - `./WTF upgrade PROJECT-NAME`
    - Do this after update command. Will perform the actual changes to the client's project.
  - `./WTF commit PROJECT-NAME`
    - Set's up the changes between the server and client's version of the project. Must be performed BEFORE push
  - `./WTF push PROJECT-NAME`
    - Send's all changes between the server and the client to the server.
    
 Update + Upgrade changes the client project with the server's version
 Commit + Push changes the server's project with the client's version
                     
                                                                                    
