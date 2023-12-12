var fs = require('fs')
var request = require('request');
var exec = require('child_process').exec;
var arguments = process.argv.splice(2);
var jsonkey = arguments[1];

function downloadFile(uri,filename,callback){
    var stream = fs.createWriteStream(filename);
    request(uri).pipe(stream).on('close', callback); 
}
fs.readFile(arguments[0], function (error, data) {
  if (error) {
    console.log('error')
  } else {
    
	var jsonobj = JSON.parse(data.toString());
	for (var jsonkey in jsonobj.WinCXDependencies) {
		if(jsonkey)
		{
			var urlStr = jsonobj.WinCXDependencies[jsonkey];
			console.log(urlStr);
			var filename = urlStr.substring(urlStr.lastIndexOf('/')+1)
			downloadFile(urlStr,filename,function(){
				console.log(filename+'download successed');
				 exec("msiexec.exe /i "+filename+" /qn", function(err, data) {  
					if(err==null)
					{
						console.log("install successed");   
					}
					                    
				});  
			});
		}else{
			console.log("error");
		}
	}
	
  }
})