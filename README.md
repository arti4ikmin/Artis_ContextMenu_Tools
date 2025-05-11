# Arti's Context Menu Tools

Tools for the windows explorer context menu (right click)

#### Currently has following tools:
- Change File Extension (e.g. an images webp -> png)
- Upload a file to TmpFiles and auto copy link (easily share)
- Copy File Path(s) (a better form of the already existing feature, without quotes and normal slashes)
- Show File Hashes (MD5 and SHA256 currently)
  more to come...
  
![Screenshot](https://github.com/user-attachments/assets/c411e8e6-bbca-4798-9f6b-845f28a78017)


### Details
Project compiles to a COM (SHARED) lib

Can be integrated with Regsvr32 <dllname> and unregistered adding a /u arg (requires admin)

Currently OpenCV is the only required  external lib

Project relies a lot on MsgBoxes and native windows dialogs

If you want to change the name of the menu item from Arti`s Tools simply search for it in the project, and change the string to anything desired

### ToDo and Plans
- More features (If I think of more)
- More extension change options
- somehow reduce the additional libs required for the dll to function
- Add QR code when uploading to tmp files


