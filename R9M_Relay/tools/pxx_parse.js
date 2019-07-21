var INFO = true;
var DEBUG = false;

var fName = WScript.Arguments(0);
info('Reading file: '+fName);

var fso = new ActiveXObject("Scripting.FileSystemObject");
var ForReading = 1;
var file = fso.OpenTextFile(fName, ForReading);
file.ReadLine(); // Always skip 1st line as a header;

var packet = null;
var packets = [];
var prevTime = null; 
var prevValue = null;

var lineCount = 0;
while (!file.AtEndofStream) {
  lineCount++;
  var line = file.ReadLine();
  log(lineCount+'> '+line);
  if (line == null || line.length == 0)
    continue;
  var fields = line.split(',');
  if (fields.length != 2) {
    error('line '+lineCount+' has wrong number of fields: '+line); 
    continue;
  }
  var time = round(parseFloat(trim(fields[0]))*1e6);
  if (time < 0) {
    error('line '+lineCount+' has time before zero: "'+time+'"'); 
    continue;
  }
  var value = trim(fields[1]);
  if (value != '0' && value != '1') {
    error('line '+lineCount+' has wrong signal: "'+value+'"'); 
    continue;
  }
  value = value == '0' ? 1 : 0; // inverted;
  var delta = null;
  if (prevTime != null) {
    if (prevValue == value) {
      error('line '+lineCount+' has same signal as before: "'+value+'"'); 
    } else {
      delta = round(time-prevTime);
      if (value == 1 && delta > 100) {
        if (packet != null) {
          var duration = round(prevTime-packet.startTime);
          info('------------- Packet stop: '+packets.length+1+', lines: '+packet.lines.length+', duration [uS]: '+duration+', delta [uS]: '+delta);
        }
        log('------------- Packet start: '+packets.length+1);
        packet = {lines: [], startTime: time};
        packets.push(packet);
      }
    }
  }
  if (packet != null) {
    packet.lines.push({time: time, value: value});
  }
  log ('time [uS]: '+time+', value: '+value+', prev: '+prevTime+
     (delta != null ? ', Delta[uS]: '+delta : ''));    
  prevTime = time;
  prevValue = value;
}

file.close();

var dirName = fName.substr(0,fName.lastIndexOf('.'));
info ('Output folder: '+dirName+', Exists: '+fso.FolderExists(dirName));
if (fso.FolderExists(dirName)) {
   info('Delete folder: '+dirName);
   var folder = fso.getFolder(dirName);
   folder.Delete(false);
}
info('Create folder: '+dirName);
var folder = fso.createFolder(dirName);
for (var i=0; i<packets.length-1; i++) { // Skipping last incompleted packet;
  writePackageFile(dirName, i+1, packets[i]);
  writeDecodedPackageFile(dirName, i+1, packets[i]);
}

function writePackageFile(dirName, n, packet) {
  var file = createPacketFile(dirName, 'raw', n);
  for (var i=0; i<packet.lines.length; i++) {
    var line = packet.lines[i];
    var delta = i == 0 ? 0 : line.time-packet.lines[i-1].time;
    file.WriteLine(line.time-packet.startTime+','+line.value+','+delta);
  }
  file.Close();
}

function writeDecodedPackageFile(dirName, n, packet) {
  var file = createPacketFile(dirName, 'decoded', n);
  var bits = [];
  var delta_0 = 0;
  for (var i=1; i<packet.lines.length; i++) {
    var line = packet.lines[i];
    if ((i % 2 != 0) && line.value == 1) {
      error('Packet '+n+', line '+i+' - wrong signal: '+line.value);
      break;
    }
    var prevLine = packet.lines[i-1];
    var delta = line.time-prevLine.time;
    if (line.value == 1) { // _/-
      log('LOW '+delta_0+', HIGH '+delta+', LOW+HIGH='+Math.round(delta+delta_0));
      delta += delta_0;
       
      var is_0 = Math.abs(delta-16) <= 1;
      var is_1 = Math.abs(delta-24) <= 1;
      if (!is_0 && !is_1) {
         error('Packet '+n+', line '+i+' - wrong LOW+HIGH signal duration: '+delta);
      }
      bits.push(is_0 ? '0' : '1');
    } else { // -\_
      delta_0 = delta; 
      // if (Math.abs(delta-10) > 1) {
      //   error('Packet '+n+', line '+i+' - wrong LOW signal duration: '+delta);
      // }
    }
  }

  bits.push('0'); // TODO Research, is that bit a bound between packets;

  var S = {s: bits.join('') };
  file.WriteLine(S.s);

  if (!startsWith(S.s,'01111110')) {
    error('Packet '+n+', invalid header: '+S.s.substr(0,8));
  }
  if (!endsWith(S.s,'01111110')) {
    error('Packet '+n+', invalid footer: '+S.s.substr(S.s.length-8,8));
  }

  S.s = S.s.substr(8,S.s.length-8*2);
  file.WriteLine(S.s);

  // Replace 1111101 to 111111;
  var s1 = [];
  var count_1 = 0;
  for (var i=0; i<S.s.length; i++) {
    var c = S.s.charAt(i);
    if (c == '1') {
      count_1++;
      if (count_1 > 5) {
        error('Packet '+n+', char '+i+', more than 5 1s in a row: '+count_1);
      }
    } else {
      if (count_1 == 5) {
        info('Packet '+n+', skipping 0 char '+i+' after 5 1s in a row');
        count_1 = 0;
        continue;
      }
      count_1 = 0;
    }
    s1.push(c);
  }
  s1 = s1.join('');
  if (s1 != S.s) {
    file.WriteLine("FIXED");
    S.s = s1;   
    file.WriteLine(S.s);
  } else {
    file.WriteLine();
    file.WriteLine();
  }

  file.WriteLine('Length '+S.s.length);

  if (S.s.length % 8 != 0) {
    error('Packet '+n+', invalid raw length (without Header and Footer): '+bits.length);
  }

  var rx_num = cutByte(S);
  file.WriteLine('rx_num '+rx_num);

  var flag1 = cutBin(S);
  file.WriteLine('flag1 '+flag1);
  flag1 = parseBin(flag1);

  var PXX_SEND_BIND = 1;
  var PXX_SEND_FAILSAFE = 1 << 4;
  var PXX_SEND_RANGECHECK = 1 << 5;
  var CCODE_SHIFT = 1;
  var CCODE_MASK = 3;
  var PROTO_SHIFT = 6;
  var PROTO_MASK = 3;

  if (flag1 & PXX_SEND_BIND) {
     var cCode = (flag1 >> CCODE_SHIFT) & CCODE_MASK;
     var country = null;
     switch (cCode) {
       case 0: country = 'US(0)'; break;
       case 1: country = 'JP(1)'; break;
       case 2: country = 'EU(2)'; break;
       default: country = cCode;
     }
     file.WriteLine('   BIND '+country);
  }

  if (flag1 & PXX_SEND_FAILSAFE) {
     file.WriteLine('   FAILSAFE');
  }

  if (flag1 & PXX_SEND_RANGECHECK) {
     file.WriteLine('   RANGECHECK');
  }

  var proto = (flag1 >> PROTO_SHIFT) & PROTO_MASK;
  var protoName = null;
  switch (proto) {
    case 0: protoName = 'X16(0)'; break;
    case 1: protoName = 'D8(1)'; break;
    case 2: protoName = 'LR12(2)'; break;
    default: protoName = proto;
  }
  file.WriteLine('   PROTO '+protoName);

  var flag2 = cutBin(S);
  file.WriteLine('flag2 '+flag2);

  var channels = [];
  for (var i=0; i<4; i++) {
    var a1 = cutBin(S);
    var a2 = cutBin(S);
    var a3 = cutBin(S);
    file.WriteLine(a1+' '+a2+' '+a3);

    a1 = parseBin(a1);
    a2 = parseBin(a2);
    a3 = parseBin(a3);
    var c1 = a1 | ( (a2 & 15) << 8);
    var c2 = ((a2 >> 4) & 15) | (a3 << 4);
    file.WriteLine('   '+c1+' '+c2);
  }

  var extra_flags = cutBin(S);
  file.WriteLine('extra_flags '+extra_flags);
  
  var EXTERNAL_ANTENNA = 1 << 0;
  var RX_TELEM_OFF = 1 << 1;
  var RX_CHANNEL_9_16 = 1 << 2;
  var POWER_SHIFT = 3;
  var POWER_MASK = 3;
  var DISABLE_S_PORT = 1 << 5;
  var R9M_EUPLUS = 1 << 6; 

  extra_flags = parseBin(extra_flags);
  if (extra_flags & EXTERNAL_ANTENNA) {
     file.WriteLine('   EXTERNAL_ANTENNA');
  }

  if (extra_flags & RX_TELEM_OFF) {
     file.WriteLine('   RX_TELEM_OFF');
  }

  if (extra_flags & RX_CHANNEL_9_16) {
     file.WriteLine('   RX_CHANNEL_9_16');
  }

  var power = (extra_flags >> POWER_SHIFT) & POWER_MASK;
  file.WriteLine('   POWER '+power);

  if (extra_flags & DISABLE_S_PORT) {
     file.WriteLine('   DISABLE_S_PORT');
  }

  if (extra_flags & R9M_EUPLUS) {
     file.WriteLine('   R9M_EUPLUS');
  }

  var crc_high = cutByte(S);
  var crc_low = cutByte(S);
  file.WriteLine('crc '+crc_high+' '+crc_low);

  file.Close();
}

function createPacketFile(dirName, ext, n) {
  var fileName = n+'';
  fileName = '0000'.substr(fileName.length)+fileName;
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

function cutBin(S) {
  var b = S.s.substr(0,8);
  S.s = S.s.substr(8);
  return b;
}

function parseBin(s) {
  return parseInt(s,2); // reverse;
}

function cutByte(S) {
  return parseBin(cutBin(S));
}

function endsWith(s, str) {
  return s.substr(s.length-str.length,str.length) == str;
}

function reverse(s) {
  return s.split('').reverse().join('');
}

function round(x) {
  // x = Math.round(x);
  return x;
}