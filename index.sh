
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


# echo "<style>body{text-align:center;overflow-y:scroll;font:calc(0.75em + 1vmin) monospace}pre pre{text-align:left;display:inline-block}img{max-width:57ch;display:block;height:auto;width:100%}@media(prefers-color-scheme:dark){body{background:#000;color:#fff}a{color:#6CF}}</style>"

echo "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"

echo "</head>"

echo "<body>"
echo "<div class="header" id="myHeader">"
#</div>"

echo "<h1> Oletko valmiina hakkeroimaan? </h1>"
# echo "Oletko valmiina hakkeroimaan?"

echo "</div>"
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


echo "<canvas class=\"game\" id=\"canvas\" oncontextmenu=\"event.preventDefault()\""
echo "style=\"width: 100%; height:100%;\"></canvas>"

#echo "<div include-html="background.html"></div> "
#
#
#
#echo "<script>"
#echo "function includeHTML() {"
#echo "var z, i, elmnt, file, xhttp;"
#echo "/*loop through a collection of all HTML elements:*/"
#echo "z = document.getElementsByTagName(\"*\");"
#echo "for (i = 0; i < z.length; i++) {"
#echo "elmnt = z[i];"
#echo "/*search for elements with a certain atrribute:*/"
#echo "file = elmnt.getAttribute(\"include-html\");"
#echo "if (file) {"
#echo "     /*make an HTTP request using the attribute value as the file name:*/"
#echo "     xhttp = new XMLHttpRequest();"
#echo "     xhttp.onreadystatechange = function() {"
#echo "       if (this.readyState == 4) {"
#echo "         if (this.status == 200) {elmnt.innerHTML = this.responseText;}"
#echo "         if (this.status == 404) {elmnt.innerHTML = \"Page not found.\";}"
#echo "         /*remove the attribute, and call this function once more:*/"
#echo "         elmnt.removeAttribute(\"include-html\");"
#echo "         includeHTML();"
#echo "       }"
#echo "     }      "
#echo "     xhttp.open(\"GET\", file, true);"
#echo "     xhttp.send();"
#echo "     /*exit the function:*/"
#echo "     return;"
#echo "   }"
#echo " }"
#echo "};"
#echo "</script>"

# echo "<script>"
# echo "includeHTML();"
# echo "</script>"

 echo "<script type=\"text/javascript\" src=\"init_canvas.js\"> </script>"
# echo "<script type=\"text/javascript\" src=\"test.js\"> </script>"
# echo "<script type=\"text/javascript\" src=\"index.js\"> </script>"

# echo "<script>"
# echo "  console.log(\"asd\")"
# echo "</script>"



echo "</body>"

