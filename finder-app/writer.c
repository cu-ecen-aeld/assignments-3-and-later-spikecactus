#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    if(argc != 3)
    {
        syslog(LOG_ERR, "Format: %s <writefile> <writestr>", argv[0]);
        closelog();
        exit(1);
    }

    char *writefile = argv[1];
    char *writestr = argv[2];

    FILE *file = fopen(writefile, "w");
    if (file == NULL)
    {
        syslog(LOG_ERR, "Cannot open file \"%s\".  Errno: %d", writefile, errno);   
        closelog();
        exit(1);
    } 

    fprintf(file, "%s\n", writestr);
    fclose(file);

    syslog(LOG_DEBUG, "Succesfully written \"%s\" to %s", writestr, writefile);
    return 0;
}