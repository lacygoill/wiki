    tar -xvf /path/to/foo.tar       To extract an uncompressed archive:

    tar -cvf /path/to/foo.tar /path/to/foo/
                                    To create an uncompressed archive:

    tar -xzvf /path/to/foo.tgz      To extract a .gz archive:

    tar -czvf /path/to/foo.tgz /path/to/foo/
                                    To create a .gz archive:

    tar -ztvf /path/to/foo.tgz      To list the content of an .gz archive:

    tar -xjvf /path/to/foo.tgz      To extract a .bz2 archive:

    tar -cjvf /path/to/foo.tgz /path/to/foo/
                                    To create a .bz2 archive:

    tar -jtvf /path/to/foo.tgz      To list the content of an .bz2 archive:

    tar czvf /path/to/foo.tgz --exclude=\*.{jpg,gif,png,wmv,flv,tar.gz,zip} /path/to/foo/

                                    To create a .gz archive and exclude all jpg,gif,... from the tgz
