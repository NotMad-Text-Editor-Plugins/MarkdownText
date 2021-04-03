import os
from os.path import *
from zipfile import *

os.system('del filelist')

os.system('del MDRes.zip')

os.system('dir /s /b >> filelist')

root = split(realpath(__file__))[0]
package = ZipFile(join(root, "MDRes.zip"),
 "w"
 #, ZIP_LZMA
 , ZIP_DEFLATED
 , compresslevel=9)

mjs="main.js"

filename = join(root, mjs)
if isfile(filename):
	f = open('filelist', 'r')
	package.write(filename, mjs)
	for pathname in f:
		pathname = pathname.strip()
		filename = pathname[len(root)+1:]
		if filename!="filelist" and filename!=".gitignore" and filename!=mjs:
			if len(filename)>0 and isfile(pathname):
				package.write(pathname, filename)
				print(filename)
else:
	print("Requires main.js from md.html !")
	print("Please download : https://makenowjust.github.io/md.html/main.js")
	os.system("pause")

package.close()
