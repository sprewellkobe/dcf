#!bash
rsync --progress 60.28.175.211::vrdata/libmcf.so .
rsync --progress 60.28.175.211::vrdata/stopwords .
cp ./libmcf.so /usr/local/apache2/htdocs/
