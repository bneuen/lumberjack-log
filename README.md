# lumberjack-log
Utility to chop log ouptut into smaller log files, and manages rotating those log files

Currently, no build system; so just build with the following (or similar):
```
gcc lumberjack.c -o lumberjack
```

Current Usage:
```
Usage: <some_binary> 2>&1 | ./lumberjack [OPTION]...
       ./lumberjack [OPTION]...
Chop log into smaller logs.

  -a          append existing log output
  -d          add local datetime stamp at the start of each line
  -f FILENAME filename to use (default is log.log)
  -h          print this usage and exit
  -l LINES    maximum number of lines per file (default is 10000)
  -n FILES    maximum number of files to maintain (default is 10)
  -t          add epoch timestamp at the start of each line
```
