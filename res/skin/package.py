import os
from os.path import *
from zipfile import *

os.system('del filelist')

os.system('del MDRes.zip')

os.system('dir /s /b >> filelist')

root = split(realpath(__file__))[0]

MDEngRepo = input("Input Where is main.js and ui.js : ")

if len(MDEngRepo)==0:
	MDEngRepo="D:\\Code\\FigureOut\\chrome\\extesions\\MarkdownEngines"

if not isdir(MDEngRepo):
	MDEngRepo = root

mjs="main.js"
ui="ui.js"
darmode="darkmode.js"

mjs_file = join(MDEngRepo, mjs)
ui_file =  join(MDEngRepo, ui)
darmode_file =  join(MDEngRepo, darmode)
if isfile(mjs_file) and isfile(ui_file) and isfile(darmode_file):
	package = ZipFile(join(root, "MDRes.zip"),
	 "w"
	 #, ZIP_LZMA
	 , ZIP_DEFLATED
	 , compresslevel=9)
	f = open('filelist', 'r')
	package.write(ui_file , ui )
	package.write(mjs_file, mjs)
	package.write(darmode_file, darmode)
	for pathname in f:
		pathname = pathname.strip()
		filename = pathname[len(root)+1:]
		if filename!="filelist" and filename!=".gitignore" and filename!=mjs and filename!=ui:
			if len(filename)>0 and isfile(pathname):
				package.write(pathname, filename)
				print(filename)
	package.close()
else:
	print("\n\nError!!!\n\nRequire main.js and ui.js from the MarkdownEngines repository.")
	print("\nPlease download : https://github.com/KnIfER/Extesions/tree/master/MarkdownEngines\n")
	os.system("pause")

