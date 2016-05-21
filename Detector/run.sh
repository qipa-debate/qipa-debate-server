file_name=$1
echo ${file_name}
if [ -f "${file_name}.pcm" ]; then
  rm ${file_name}.content
  java -jar SpeedUnderstand.jar ${file_name}.pcm ${file_name}.content ${file_name}.words ./stopword.dic
fi
