file_name=$1
echo ${file_name}
if [ -f "${file_name}.pcm" ]; then
  rm ${file_name}.content
  echo "$file_name"
  java -jar SpeedUnderstand.jar ${file_name}.pcm ${file_name}.content ${file_name%_*}.words ./stopword.dic
  java -jar TopicModel.jar ${file_name%_*}.words
fi
