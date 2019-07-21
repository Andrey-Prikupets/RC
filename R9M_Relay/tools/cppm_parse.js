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
var prevDelta = null;

var lineCount = 0;
var channels = null;
var channelCount = 0;
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

  if (delta > 2500) {
    log('Time: '+time+', Init packet, delta='+delta);      
    // Out channels;
    if (channels != null) {
      packets.push (channels);
    }
    channelCount = 0;
    channels = [];
  } else {
    if (channels == null)
      continue;
  }

  if (value == 0) { // 0 = -\_
    if (channelCount > 0) {
      log('Time: '+time+', value=0, channelCount='+channelCount);
      if (channelCount > 9) {
        error('Time: '+time+', packet: '+packets.length+', More than 9 channels: '+channelCount);
      }
      var v = prevDelta + delta; // add 2nd part;
      channels[channelCount-1] = v;
      log('Time: '+time+', packet: '+packets.length+', channels['+channelCount+']='+v);      
      if (v < 1000) {
        error('Time: '+time+', packet: '+packets.length+', channels['+channelCount+'] is too low: '+v);
      }
      if (v > 2000) {
        error('Time: '+time+', packet: '+packets.length+', channels['+channelCount+'] is too high: '+v);
      }
    }  
  } else { // 1 = _/-
    log('Time: '+time+', value=1, channelCount++');
    channelCount++;
    prevDelta = delta; // save 1st part (300-400us);
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
  file.WriteLine(i+','+packets[i].join(','));
}
file.Close();

function createPacketFile(dirName, ext, n) {
  fileName = 'cppm_'+n;
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