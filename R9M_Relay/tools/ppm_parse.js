var INFO = true;
var DEBUG = false;

var fName = WScript.Arguments(0);
var channelNum = parseInt(WScript.Arguments(1), 10);
info('Reading file: '+fName+', channel: '+channelNum);

var fso = new ActiveXObject("Scripting.FileSystemObject");
var ForReading = 1;
var file = fso.OpenTextFile(fName, ForReading);
file.ReadLine(); // Always skip 1st line as a header;

var prevTime = null; 
var prevValue = null;
var prevErrorTime = null;

var lineCount = 0;
var packets = [];

while (!file.AtEndofStream) {
  lineCount++;
  var line = file.ReadLine();
  log(lineCount+'> '+line);
  if (line == null || line.length == 0)
    continue;
  var fields = line.split(',');
  if (fields.length-1 <= channelNum) {
    error('line '+lineCount+' has wrong number of fields: '+line); 
    continue;
  }
  var time = round(parseFloat(trim(fields[0]))*1e6);
  if (time < 0) {
    error('line '+lineCount+' has time before zero: "'+time+'"'); 
    continue;
  }
  var value = trim(fields[channelNum+1]);
  if (value != '0' && value != '1') {
    error('line '+lineCount+' has wrong signal: "'+value+'"'); 
    continue;
  }
  value = value == '0' ? 0 : 1; 
  if (prevTime == null) {
    prevTime = time;
    prevValue = value;
    continue;
  }
  if (prevValue == value) {
    continue;
  }

  delta = round(time-prevTime);
  log ('time [uS]: '+time+', value: '+value+', prev: '+prevTime+
     (delta != null ? ', Delta[uS]: '+delta : ''));    

  prevTime = time;
  prevValue = value;

  if (value == 0) { // 0 = -\_
    if (prevValue != null) {
      log('Time: '+time+', value=0');
      if (delta < 1000) {
        var errorDelta = prevErrorTime == null ? '' : ', error delta: '+(time-prevErrorTime)/1000;
        prevErrorTime = time;
        error('Time: '+time+', packet: '+packets.length+', value is too low: '+delta+errorDelta);
      }
      if (delta > 2000) {
        var errorDelta = prevErrorTime == null ? '' : ', error delta: '+(time-prevErrorTime)/1000;
        prevErrorTime = time;
        error('Time: '+time+', packet: '+packets.length+', value is too high: '+delta+errorDelta);
      }
      packets.push(delta);
    }  
  } else { // 1 = _/-
    log('Time: '+time+', value=1');
    if (delta < 2500) {
      error('Time: '+time+', packet: '+packets.length+', gap is too small: '+delta);
    }
  }
}

file.close();

var dirName = fName.substr(0,fName.lastIndexOf('.'));
info ('Output folder: '+dirName+', Exists: '+fso.FolderExists(dirName));
if (!fso.FolderExists(dirName)) {
  info('Create folder: '+dirName);
  fso.createFolder(dirName);
}
var folder = fso.getFolder(dirName);

var file = createPacketFile(dirName, 'csv', channelNum);
for (var i=0; i<packets.length-1; i++) { // Skipping last incompleted packet;
  file.WriteLine(i+','+packets[i]);
}
file.Close();

function createPacketFile(dirName, ext, n) {
  fileName = 'ppm_'+n;
  var fName = dirName+'\\'+fileName+'.'+ext;
  info('Create file: '+fName);
  return fso.CreateTextFile(fName);
}

function log(s) {
  if (DEBUG) WScript.Echo(s);
}

function info(s) {
  if (INFO) WScript.Echo(s);
}

function error(s) {
  WScript.Echo('ERROR: '+s);
}

function trim(str) {
  return str.replace(/^\s+|\s+$/g, '');
}

function startsWith(s, str) {
  return s.substr(0,str.length) == str;
}

function endsWith(s, str) {
  return s.substr(s.length-str.length,str.length) == str;
}

function round(x) {
  x = Math.round(x);
  return x;
}