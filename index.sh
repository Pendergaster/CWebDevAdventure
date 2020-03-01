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
echo "<h1> tervetuloa sivulle</h1>"
echo "<br>"
echo "terveisia unit_test.sh:sta"
echo "<br>"
echo "<br>"



echo "<form action=\"/compile\" method=\"post\" target=\"_blank\"> "
echo "<textarea name=\"message\" id="message" rows="6" cols="75">Code here</textarea>"
echo "<br><br>"
echo "<input type=\"submit\" value=\"compile\">"
echo "</form>"


echo "<br>"
echo "<br>"

# echo "<img src=\"minion.jpg\" alt=\"Italian minion\">"

echo "<br>"
echo "<br>"

echo "</body>"

