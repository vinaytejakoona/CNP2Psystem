clientlist="../clientlist"
cd publishedFiles
ls | while IFS='' read -r filename || [[ -n "$filename" ]]; 
do
  awk  '/songs/ {print  FILENAME" "$0 }' $filename >> ../searchResults
done

cat $clientlist | while read filename ip port || [[ -n "$filename" ]];
do
 echo $filename
 sed -i "s/$filename/$filename $ip $port/" ../searchResults
done

