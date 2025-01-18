import sys
import shutil
import os
import json
import zipfile
from pathlib import Path

# print("+++++++++++++++++++++++++")
# print("+++++++++++++++++++++++++")
# print("Run rename.py")
# print("+++++++++++++++++++++++++")
# print("+++++++++++++++++++++++++")

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	version = data['version']
except:
	version = '0.0.0'

# print("Version " + version)

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	source_file = './build/'+ data['sketch'] + '.hex'
	source_file_full = './build/'+ data['sketch'] + '_full.hex'
	full_name = data['sketch'] + '_full.hex'
	bin_name = data['sketch'] + '.bin'
	nrf_zip_file = data['sketch'] + '.zip'
except:
	source_file = '*.hex'
	bin_name = '*.bin'
	nrf_zip_file = '*.zip'

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	board_type = data['board']
except:
	board_type = 'rak'

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	project_name = data['project']
except:
	board_type = 'RUI3'

# print("Source " + source_file)
# print("Binary " + bin_name)
# print("ZIP " + nrf_zip_file)
# print("Board " + board_type)
# print("Full ZIP " + full_name)

# Specify the source file, the destination for the copy, and the new name
if board_type.find('RAK3172') != -1:
	destination_directory = './generated_rak3172/'
elif board_type.find('RAK4631') != -1:
	destination_directory = './generated_rak4631/'
else:
	destination_directory = './generated/'

new_file_name = project_name+'_V'+version+'.hex'
new_zip_name = project_name+'_V'+version+'.zip'
new_full_name = project_name+'_V'+version+'_BL.hex'

# print("New file name " + new_file_name)
# print("New ZIP name " + new_zip_name)
# print("New Full name " + new_full_name)

if not os.path.exists(destination_directory):
	try:
		os.makedirs(destination_directory)
	except:
		print('Cannot create '+destination_directory)
	

if os.path.isfile(destination_directory+new_file_name):
	try:
		os.remove(destination_directory+new_file_name)
	except:
		print('Cannot delete '+destination_directory+new_file_name)
	# finally:
	# 	print('Delete '+destination_directory+new_file_name)

if os.path.isfile(destination_directory+new_full_name):
	try:
		os.remove(destination_directory+new_full_name)
	except:
		print('Cannot delete '+destination_directory+new_full_name)
	# finally:
	# 	print('Delete '+destination_directory+new_full_name)

if os.path.isfile(destination_directory+new_zip_name):
	try:
		os.remove(destination_directory+new_zip_name)
	except:
		print('Cannot delete '+destination_directory+new_zip_name)
	# finally:
	# 	print('Delete '+destination_directory+new_zip_name)

if os.path.isfile('./build/'+new_zip_name):
	try:
		os.remove('./build/'+new_zip_name)
	except:
		print('Cannot delete '+'./build/'+new_zip_name)
	# finally:
	# 	print('Delete '+'./build/'+new_zip_name)

if os.path.isfile(destination_directory+new_zip_name):
	try:
		os.remove(destination_directory+new_zip_name)
	except:
		print('Cannot delete '+destination_directory+new_zip_name)
	# finally:
	# 	print('Delete '+'./generated/'+new_zip_name)

# Copy the files
# if board_type.find('RAK4631') != -1:
# 	try:
# 		shutil.copy2(source_file, destination_directory)
# 		print('Copied '+source_file +' to '+destination_directory)
# 	except:
# 		print('Cannot copy '+source_file +' to '+destination_directory)
# 	try:
# 		shutil.copy2(source_file_full, destination_directory)
# 		print('Copied '+source_file_full +' to '+destination_directory)
# 	except:
# 		print('Cannot copy '+source_file_full +' to '+destination_directory)

# Get the base name of the source file
base_name = os.path.basename(source_file)

# print("Base name " + base_name)

# Construct the paths to the copied file and the new file name
copied_file = os.path.join(destination_directory, base_name)
new_file = os.path.join(destination_directory, new_file_name)
new_full_file = os.path.join(destination_directory, new_full_name)
zip_name = project_name+'_V'+version+'.zip'
hex_name = project_name+'_V'+version+'.hex'
full_hex_name = project_name+'_V'+version+'_BL.hex'
# print("Copied file " + copied_file)
# print("Base name " + new_file)
# print("ZIP name " + zip_name)
# print("ZIP content " + bin_name) 

# Create ZIP file for WisToolBox
if board_type.find('RAK3172') != -1:
	try:
		os.chdir("./build")
		# print('Changed dir to ./build')
	except:
		print('Cannot change dir to ./build')

	try:
		zipfile.ZipFile(zip_name, mode='w').write(bin_name)
		# print('Zipped '+bin_name +' to '+zip_name)
	except:
		print('Cannot zip '+bin_name +' to '+zip_name)

	os.chdir("../")

	try:
		shutil.copy2("./build/"+zip_name, destination_directory)
		# print('Copied '+"./build/"+zip_name +' to '+destination_directory)
	except:
		print('Cannot copy '+"./build/"+zip_name +' to '+destination_directory)

	# Rename the file
	try:
		os.rename(copied_file, new_file)
		# print('Renamed '+copied_file +' to '+new_file)
	except:
		print('Cannot rename '+copied_file +' to '+new_file)

	try:
		shutil.copy2(source_file, new_file)
		# print('Copied '+source_file +' to ' + new_file)
	except:
		print('Cannot copy '+source_file +' to ' + new_file)
else:
	# Copy the files
	try:
		shutil.copy2("./build/"+nrf_zip_file, destination_directory+zip_name)
		# print('RAK4631 copied '+nrf_zip_file +' to '+destination_directory+zip_name)
	except:
		print('RAK4631 Cannot copy '+nrf_zip_file +' to '+destination_directory+zip_name)

	try:
		shutil.copy2(source_file, destination_directory+hex_name)
		# print('RAK4631 copied '+source_file +' to ' + destination_directory+hex_name)
	except:
		print('RAK4631 Cannot copy '+source_file +' to ' + destination_directory+hex_name)

	try:
		shutil.copy2(source_file_full, new_full_file)
		# print('RAK4631 copied'+source_file_full +' to ' + new_full_file)
	except:
		print('RAK4631 Cannot copy '+source_file_full +' to ' + new_full_file)

print("++++++++++++++++++++++++++++++++++++++++++++++++++")
print("Generated distribution files")
print(*Path(destination_directory).iterdir(), sep="\n")
print("++++++++++++++++++++++++++++++++++++++++++++++++++")
