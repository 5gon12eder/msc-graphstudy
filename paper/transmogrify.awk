#! /usr/bin/awk -f
#! -*- coding:utf-8; mode:awk; -*-

# Copyright (C) 2018 Karlsruhe Institute of Technology
# Copyright (C) 2018 Moritz Klammler <moritz.klammler@alumni.kit.edu>
#
# Copying and distribution of this file, with or without modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.  This file is offered as-is, without any warranty.

BEGIN {
    intoBusiness = 0;
    if (OUTFILE) {
        printf("transmogrify: Writing output to to '%s' ...\n", OUTFILE) > "/dev/stderr";
    } else {
        OUTFILE = "/dev/stdout";
    }
}

END {
    close(OUTFILE);
    printf("transmogrify: Copied and converted %d records\n", intoBusiness) > "/dev/stderr";
}

# Strip comment lines
/^\s*[%]/ {
    next;
}

# Skip blank lines
/^\s*$/ {
    next;
}

# Insert blank line between records
/^\s*@/ {
    if (intoBusiness++) {
        print("") > OUTFILE;
    }
}

# Rename some fields and convert 'date' into 'year', 'month', 'day'
/^\s*\w+\s*=/ {
    if ($1 == "date") {
        if (match($3, /([0-9]+)[-]?([0-9]+)?[-]?([0-9]+)?/, datematches)) {
            if (datematches[1]) {
                printf("  year = {%d},\n", datematches[1]) > OUTFILE;
            }
            if (datematches[2]) {
                printf("  month = {%d},\n", datematches[2]) > OUTFILE;
            }
            if (datematches[3]) {
                printf("  day = {%d},\n", datematches[3]) > OUTFILE;
            }
            next;
        } else {
            printf("transmogrify: %s:%d: Cannot parse 'date' field: %s\n", FILENAME, FNR, $3) > "/dev/stderr";
            exit 1;
        }
    } else if ($1 == "journaltitle") {
        $1 = "journal";
    } else if ($1 == "institution") {
        $1 = "school";
    } else {
        $1 = $1;  # self-assignment to normalize white-space
    }
    print(" ", $0) > OUTFILE;
    next;
}

# Copy all other lines unchanged
{
    print($0) > OUTFILE;
    next;
}
