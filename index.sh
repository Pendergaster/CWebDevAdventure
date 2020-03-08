#!/bin/bash

# echo "starting unit test"

#pushd ~/stickman-engine
# git pull
# ./build.sh
#popd

# echo "great success"


echo "<!DOCTYPE HTML>"
echo "<head>"

# echo "
# 
# <style>
# body {
#   background-color: lightblue;
# }
# 
# h1 {
#   color: white;
#   text-align: center;
# }
# 
# p {
#   font-family: verdana;
#   font-size: 20px;
# }
# </style>"


echo "<style>body{text-align:center;overflow-y:scroll;font:calc(0.75em + 1vmin) monospace}pre pre{text-align:left;display:inline-block}img{max-width:57ch;display:block;height:auto;width:100%}@media(prefers-color-scheme:dark){body{background:#000;color:#fff}a{color:#6CF}}</style>"
echo "</head>"

echo "<body>"
echo "<h1> Oletko valmiina hakkeroimaan? </h1>"
# echo "Oletko valmiina hakkeroimaan?"

echo "<audio controls>"
echo "<source src="audio.wav" type="audio/wav">"
echo "Your browser does not support the audio element."
echo "</audio>"


echo "<form action=\"/compile\" method=\"post\" target=\"_blank\"> "
echo "<input type=\"submit\" value=\"compile\">"
echo "<br>"
echo "<textarea name=\"message\" id="message" rows="40" cols="80">"
echo "#include<stdio.h> "
echo ""
echo "int main() {"
echo "    printf(\"hello world\\n\");"
echo "}"
echo "</textarea>"
echo "<br><br>"
echo "</form>"


# echo "<br>"
# echo "<br>"
# echo "<img src=\"minion.jpg\" alt=\"Italian minion\""
# echo "<br>"
# echo "<br>"

echo "</body>"

