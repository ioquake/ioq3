var LibrarySysCommon = {
	$CRC32: {
		TABLE: [
			0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
			0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
			0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
			0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
			0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
			0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
			0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
			0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
			0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
			0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
			0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
			0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
			0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
			0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
			0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
			0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
			0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
			0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
			0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
			0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
			0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
			0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
			0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
			0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
			0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
			0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
			0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
			0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
			0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
			0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
			0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
			0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
		],
		Start: function () {
			return -1;
		},
		Update: function (crc, buffer, offset, len) {
			for (var i = offset, l = offset + len; i < l; i++) {
				crc = CRC32.TABLE[(crc ^ buffer[i]) & 0xff] ^ (crc >>> 8);
			}
			return crc;
		},
		Finish: function (crc) {
			return (crc ^ -1) >>> 0;
		}
	},
	$SYSC__deps: ['$Browser', '$FS', '$PATH', '$SYS', '$CRC32', 'Com_Printf', 'Com_Error', 'Com_ProxyCallback', 'Con_ToggleConsole_f'],
	$SYSC__postset: /* gunzip.min.js */ '(function() {\'use strict\';function n(e){throw e;}var p=void 0,aa=this;function r(e,c){var d=e.split("."),b=aa;!(d[0]in b)&&b.execScript&&b.execScript("var "+d[0]);for(var a;d.length&&(a=d.shift());)!d.length&&c!==p?b[a]=c:b=b[a]?b[a]:b[a]={}};var u="undefined"!==typeof Uint8Array&&"undefined"!==typeof Uint16Array&&"undefined"!==typeof Uint32Array;new (u?Uint8Array:Array)(256);var v;for(v=0;256>v;++v)for(var w=v,ba=7,w=w>>>1;w;w>>>=1)--ba;function x(e,c,d){var b,a="number"===typeof c?c:c=0,f="number"===typeof d?d:e.length;b=-1;for(a=f&7;a--;++c)b=b>>>8^y[(b^e[c])&255];for(a=f>>3;a--;c+=8)b=b>>>8^y[(b^e[c])&255],b=b>>>8^y[(b^e[c+1])&255],b=b>>>8^y[(b^e[c+2])&255],b=b>>>8^y[(b^e[c+3])&255],b=b>>>8^y[(b^e[c+4])&255],b=b>>>8^y[(b^e[c+5])&255],b=b>>>8^y[(b^e[c+6])&255],b=b>>>8^y[(b^e[c+7])&255];return(b^4294967295)>>>0}var z=[0,1996959894,3993919788,2567524794,124634137,1886057615,3915621685,2657392035,249268274,2044508324,3772115230,2547177864,162941995,2125561021,3887607047,2428444049,498536548,1789927666,4089016648,2227061214,450548861,1843258603,4107580753,2211677639,325883990,1684777152,4251122042,2321926636,335633487,1661365465,4195302755,2366115317,997073096,1281953886,3579855332,2724688242,1006888145,1258607687,3524101629,2768942443,901097722,1119000684,3686517206,2898065728,853044451,1172266101,3705015759,2882616665,651767980,1373503546,3369554304,3218104598,565507253,1454621731,3485111705,3099436303,671266974,1594198024,3322730930,2970347812,795835527,1483230225,3244367275,3060149565,1994146192,31158534,2563907772,4023717930,1907459465,112637215,2680153253,3904427059,2013776290,251722036,2517215374,3775830040,2137656763,141376813,2439277719,3865271297,1802195444,476864866,2238001368,4066508878,1812370925,453092731,2181625025,4111451223,1706088902,314042704,2344532202,4240017532,1658658271,366619977,2362670323,4224994405,1303535960,984961486,2747007092,3569037538,1256170817,1037604311,2765210733,3554079995,1131014506,879679996,2909243462,3663771856,1141124467,855842277,2852801631,3708648649,1342533948,654459306,3188396048,3373015174,1466479909,544179635,3110523913,3462522015,1591671054,702138776,2966460450,3352799412,1504918807,783551873,3082640443,3233442989,3988292384,2596254646,62317068,1957810842,3939845945,2647816111,81470997,1943803523,3814918930,2489596804,225274430,2053790376,3826175755,2466906013,167816743,2097651377,4027552580,2265490386,503444072,1762050814,4150417245,2154129355,426522225,1852507879,4275313526,2312317920,282753626,1742555852,4189708143,2394877945,397917763,1622183637,3604390888,2714866558,953729732,1340076626,3518719985,2797360999,1068828381,1219638859,3624741850,2936675148,906185462,1090812512,3747672003,2825379669,829329135,1181335161,3412177804,3160834842,628085408,1382605366,3423369109,3138078467,570562233,1426400815,3317316542,2998733608,733239954,1555261956,3268935591,3050360625,752459403,1541320221,2607071920,3965973030,1969922972,40735498,2617837225,3943577151,1913087877,83908371,2512341634,3803740692,2075208622,213261112,2463272603,3855990285,2094854071,198958881,2262029012,4057260610,1759359992,534414190,2176718541,4139329115,1873836001,414664567,2282248934,4279200368,1711684554,285281116,2405801727,4167216745,1634467795,376229701,2685067896,3608007406,1308918612,956543938,2808555105,3495958263,1231636301,1047427035,2932959818,3654703836,1088359270,936918E3,2847714899,3736837829,1202900863,817233897,3183342108,3401237130,1404277552,615818150,3134207493,3453421203,1423857449,601450431,3009837614,3294710456,1567103746,711928724,3020668471,3272380065,1510334235,755167117],y=u?new Uint32Array(z):z;function A(){}A.prototype.getName=function(){return this.name};A.prototype.getData=function(){return this.data};A.prototype.G=function(){return this.H};r("Zlib.GunzipMember",A);r("Zlib.GunzipMember.prototype.getName",A.prototype.getName);r("Zlib.GunzipMember.prototype.getData",A.prototype.getData);r("Zlib.GunzipMember.prototype.getMtime",A.prototype.G);function C(e){var c=e.length,d=0,b=Number.POSITIVE_INFINITY,a,f,g,k,m,q,t,h,l;for(h=0;h<c;++h)e[h]>d&&(d=e[h]),e[h]<b&&(b=e[h]);a=1<<d;f=new (u?Uint32Array:Array)(a);g=1;k=0;for(m=2;g<=d;){for(h=0;h<c;++h)if(e[h]===g){q=0;t=k;for(l=0;l<g;++l)q=q<<1|t&1,t>>=1;for(l=q;l<a;l+=m)f[l]=g<<16|h;++k}++g;k<<=1;m<<=1}return[f,d,b]};var D=[],E;for(E=0;288>E;E++)switch(!0){case 143>=E:D.push([E+48,8]);break;case 255>=E:D.push([E-144+400,9]);break;case 279>=E:D.push([E-256+0,7]);break;case 287>=E:D.push([E-280+192,8]);break;default:n("invalid literal: "+E)}var ca=function(){function e(a){switch(!0){case 3===a:return[257,a-3,0];case 4===a:return[258,a-4,0];case 5===a:return[259,a-5,0];case 6===a:return[260,a-6,0];case 7===a:return[261,a-7,0];case 8===a:return[262,a-8,0];case 9===a:return[263,a-9,0];case 10===a:return[264,a-10,0];case 12>=a:return[265,a-11,1];case 14>=a:return[266,a-13,1];case 16>=a:return[267,a-15,1];case 18>=a:return[268,a-17,1];case 22>=a:return[269,a-19,2];case 26>=a:return[270,a-23,2];case 30>=a:return[271,a-27,2];case 34>=a:return[272,a-31,2];case 42>=a:return[273,a-35,3];case 50>=a:return[274,a-43,3];case 58>=a:return[275,a-51,3];case 66>=a:return[276,a-59,3];case 82>=a:return[277,a-67,4];case 98>=a:return[278,a-83,4];case 114>=a:return[279,a-99,4];case 130>=a:return[280,a-115,4];case 162>=a:return[281,a-131,5];case 194>=a:return[282,a-163,5];case 226>=a:return[283,a-195,5];case 257>=a:return[284,a-227,5];case 258===a:return[285,a-258,0];default:n("invalid length: "+a)}}var c=[],d,b;for(d=3;258>=d;d++)b=e(d),c[d]=b[2]<<24|b[1]<<16|b[0];return c}();u&&new Uint32Array(ca);function G(e,c){this.i=[];this.j=32768;this.d=this.f=this.c=this.n=0;this.input=u?new Uint8Array(e):e;this.o=!1;this.k=H;this.w=!1;if(c||!(c={}))c.index&&(this.c=c.index),c.bufferSize&&(this.j=c.bufferSize),c.bufferType&&(this.k=c.bufferType),c.resize&&(this.w=c.resize);switch(this.k){case I:this.a=32768;this.b=new (u?Uint8Array:Array)(32768+this.j+258);break;case H:this.a=0;this.b=new (u?Uint8Array:Array)(this.j);this.e=this.D;this.q=this.A;this.l=this.C;break;default:n(Error("invalid inflate mode"))}}var I=0,H=1;G.prototype.g=function(){for(;!this.o;){var e=J(this,3);e&1&&(this.o=!0);e>>>=1;switch(e){case 0:var c=this.input,d=this.c,b=this.b,a=this.a,f=p,g=p,k=p,m=b.length,q=p;this.d=this.f=0;f=c[d++];f===p&&n(Error("invalid uncompressed block header: LEN (first byte)"));g=f;f=c[d++];f===p&&n(Error("invalid uncompressed block header: LEN (second byte)"));g|=f<<8;f=c[d++];f===p&&n(Error("invalid uncompressed block header: NLEN (first byte)"));k=f;f=c[d++];f===p&&n(Error("invalid uncompressed block header: NLEN (second byte)"));k|=f<<8;g===~k&&n(Error("invalid uncompressed block header: length verify"));d+g>c.length&&n(Error("input buffer is broken"));switch(this.k){case I:for(;a+g>b.length;){q=m-a;g-=q;if(u)b.set(c.subarray(d,d+q),a),a+=q,d+=q;else for(;q--;)b[a++]=c[d++];this.a=a;b=this.e();a=this.a}break;case H:for(;a+g>b.length;)b=this.e({t:2});break;default:n(Error("invalid inflate mode"))}if(u)b.set(c.subarray(d,d+g),a),a+=g,d+=g;else for(;g--;)b[a++]=c[d++];this.c=d;this.a=a;this.b=b;break;case 1:this.l(da,ea);break;case 2:fa(this);break;default:n(Error("unknown BTYPE: "+e))}}return this.q()};var K=[16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15],L=u?new Uint16Array(K):K,N=[3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258,258,258],O=u?new Uint16Array(N):N,P=[0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0],Q=u?new Uint8Array(P):P,T=[1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577],ga=u?new Uint16Array(T):T,ha=[0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13],U=u?new Uint8Array(ha):ha,V=new (u?Uint8Array:Array)(288),W,ia;W=0;for(ia=V.length;W<ia;++W)V[W]=143>=W?8:255>=W?9:279>=W?7:8;var da=C(V),X=new (u?Uint8Array:Array)(30),Y,ja;Y=0;for(ja=X.length;Y<ja;++Y)X[Y]=5;var ea=C(X);function J(e,c){for(var d=e.f,b=e.d,a=e.input,f=e.c,g;b<c;)g=a[f++],g===p&&n(Error("input buffer is broken")),d|=g<<b,b+=8;g=d&(1<<c)-1;e.f=d>>>c;e.d=b-c;e.c=f;return g}function Z(e,c){for(var d=e.f,b=e.d,a=e.input,f=e.c,g=c[0],k=c[1],m,q,t;b<k;){m=a[f++];if(m===p)break;d|=m<<b;b+=8}q=g[d&(1<<k)-1];t=q>>>16;e.f=d>>t;e.d=b-t;e.c=f;return q&65535}function fa(e){function c(a,c,b){var d,e,f,g;for(g=0;g<a;)switch(d=Z(this,c),d){case 16:for(f=3+J(this,2);f--;)b[g++]=e;break;case 17:for(f=3+J(this,3);f--;)b[g++]=0;e=0;break;case 18:for(f=11+J(this,7);f--;)b[g++]=0;e=0;break;default:e=b[g++]=d}return b}var d=J(e,5)+257,b=J(e,5)+1,a=J(e,4)+4,f=new (u?Uint8Array:Array)(L.length),g,k,m,q;for(q=0;q<a;++q)f[L[q]]=J(e,3);g=C(f);k=new (u?Uint8Array:Array)(d);m=new (u?Uint8Array:Array)(b);e.l(C(c.call(e,d,g,k)),C(c.call(e,b,g,m)))}G.prototype.l=function(e,c){var d=this.b,b=this.a;this.r=e;for(var a=d.length-258,f,g,k,m;256!==(f=Z(this,e));)if(256>f)b>=a&&(this.a=b,d=this.e(),b=this.a),d[b++]=f;else{g=f-257;m=O[g];0<Q[g]&&(m+=J(this,Q[g]));f=Z(this,c);k=ga[f];0<U[f]&&(k+=J(this,U[f]));b>=a&&(this.a=b,d=this.e(),b=this.a);for(;m--;)d[b]=d[b++-k]}for(;8<=this.d;)this.d-=8,this.c--;this.a=b};G.prototype.C=function(e,c){var d=this.b,b=this.a;this.r=e;for(var a=d.length,f,g,k,m;256!==(f=Z(this,e));)if(256>f)b>=a&&(d=this.e(),a=d.length),d[b++]=f;else{g=f-257;m=O[g];0<Q[g]&&(m+=J(this,Q[g]));f=Z(this,c);k=ga[f];0<U[f]&&(k+=J(this,U[f]));b+m>a&&(d=this.e(),a=d.length);for(;m--;)d[b]=d[b++-k]}for(;8<=this.d;)this.d-=8,this.c--;this.a=b};G.prototype.e=function(){var e=new (u?Uint8Array:Array)(this.a-32768),c=this.a-32768,d,b,a=this.b;if(u)e.set(a.subarray(32768,e.length));else{d=0;for(b=e.length;d<b;++d)e[d]=a[d+32768]}this.i.push(e);this.n+=e.length;if(u)a.set(a.subarray(c,c+32768));else for(d=0;32768>d;++d)a[d]=a[c+d];this.a=32768;return a};G.prototype.D=function(e){var c,d=this.input.length/this.c+1|0,b,a,f,g=this.input,k=this.b;e&&("number"===typeof e.t&&(d=e.t),"number"===typeof e.z&&(d+=e.z));2>d?(b=(g.length-this.c)/this.r[2],f=258*(b/2)|0,a=f<k.length?k.length+f:k.length<<1):a=k.length*d;u?(c=new Uint8Array(a),c.set(k)):c=k;return this.b=c};G.prototype.q=function(){var e=0,c=this.b,d=this.i,b,a=new (u?Uint8Array:Array)(this.n+(this.a-32768)),f,g,k,m;if(0===d.length)return u?this.b.subarray(32768,this.a):this.b.slice(32768,this.a);f=0;for(g=d.length;f<g;++f){b=d[f];k=0;for(m=b.length;k<m;++k)a[e++]=b[k]}f=32768;for(g=this.a;f<g;++f)a[e++]=c[f];this.i=[];return this.buffer=a};G.prototype.A=function(){var e,c=this.a;u?this.w?(e=new Uint8Array(c),e.set(this.b.subarray(0,c))):e=this.b.subarray(0,c):(this.b.length>c&&(this.b.length=c),e=this.b);return this.buffer=e};function $(e){this.input=e;this.c=0;this.m=[];this.s=!1}$.prototype.F=function(){this.s||this.g();return this.m.slice()};$.prototype.g=function(){for(var e=this.input.length;this.c<e;){var c=new A,d=p,b=p,a=p,f=p,g=p,k=p,m=p,q=p,t=p,h=this.input,l=this.c;c.u=h[l++];c.v=h[l++];(31!==c.u||139!==c.v)&&n(Error("invalid file signature:"+c.u+","+c.v));c.p=h[l++];switch(c.p){case 8:break;default:n(Error("unknown compression method: "+c.p))}c.h=h[l++];q=h[l++]|h[l++]<<8|h[l++]<<16|h[l++]<<24;c.H=new Date(1E3*q);c.N=h[l++];c.M=h[l++];0<(c.h&4)&&(c.I=h[l++]|h[l++]<<8,l+=c.I);if(0<(c.h&8)){m=[];for(k=0;0<(g=h[l++]);)m[k++]=String.fromCharCode(g);c.name=m.join("")}if(0<(c.h&16)){m=[];for(k=0;0<(g=h[l++]);)m[k++]=String.fromCharCode(g);c.J=m.join("")}0<(c.h&2)&&(c.B=x(h,0,l)&65535,c.B!==(h[l++]|h[l++]<<8)&&n(Error("invalid header crc16")));d=h[h.length-4]|h[h.length-3]<<8|h[h.length-2]<<16|h[h.length-1]<<24;h.length-l-4-4<512*d&&(f=d);b=new G(h,{index:l,bufferSize:f});c.data=a=b.g();l=b.c;c.K=t=(h[l++]|h[l++]<<8|h[l++]<<16|h[l++]<<24)>>>0;x(a,p,p)!==t&&n(Error("invalid CRC-32 checksum: 0x"+x(a,p,p).toString(16)+" / 0x"+t.toString(16)));c.L=d=(h[l++]|h[l++]<<8|h[l++]<<16|h[l++]<<24)>>>0;(a.length&4294967295)!==d&&n(Error("invalid input size: "+(a.length&4294967295)+" / "+d));this.m.push(c);this.c=l}this.s=!0;var F=this.m,s,M,R=0,S=0,B;s=0;for(M=F.length;s<M;++s)S+=F[s].data.length;if(u){B=new Uint8Array(S);for(s=0;s<M;++s)B.set(F[s].data,R),R+=F[s].data.length}else{B=[];for(s=0;s<M;++s)B[s]=F[s].data;B=Array.prototype.concat.apply([],B)}return B};r("Zlib.Gunzip",$);r("Zlib.Gunzip.prototype.decompress",$.prototype.g);r("Zlib.Gunzip.prototype.getMembers",$.prototype.F);}).call(typeof window !== "undefined" ? window : global);\n' +
		/* tar.min.js */ '(function(e){function n(e){var t=[];for(var n=0;n<e.length;n++){var r=e[n];if(r===0){break}t.push(r)}return String.fromCharCode.apply(null,t)}function r(e){var t=n(e);t=parseInt(t,8);return isNaN(t)?null:t}var t=512;var i=function(e){var n=e instanceof ArrayBuffer||typeof Buffer!=="undefined"&&e instanceof Buffer;var r=e instanceof Int8Array||e instanceof Uint8Array;if(!n&&!r){throw new Error("Must specify a valid ArrayBuffer, Buffer, INT8Array or Int8Array.")}this.INT8=r?e:new Int8Array(e);this.headers={};this.pos=0;var i=0;while(i<2){var s=this.pos;var o=this._readHeader();if(!o.name){i++;continue}this.headers[o.name]=o;this.pos+=Math.ceil(o.size/t)*t}};i.prototype._readHeader=function(){var e={name:n(this.INT8.subarray(this.pos,this.pos+=100)),mode:r(this.INT8.subarray(this.pos,this.pos+=8)),uid:r(this.INT8.subarray(this.pos,this.pos+=8)),gid:r(this.INT8.subarray(this.pos,this.pos+=8)),size:r(this.INT8.subarray(this.pos,this.pos+=12)),mtime:r(this.INT8.subarray(this.pos,this.pos+=12)),chksum:r(this.INT8.subarray(this.pos,this.pos+=8)),typeflag:n(this.INT8.subarray(this.pos,this.pos+=1)),linkname:n(this.INT8.subarray(this.pos,this.pos+=100)),magic:n(this.INT8.subarray(this.pos,this.pos+=6)),version:n(this.INT8.subarray(this.pos,this.pos+=2)),uname:n(this.INT8.subarray(this.pos,this.pos+=32)),gname:n(this.INT8.subarray(this.pos,this.pos+=32)),devmajor:r(this.INT8.subarray(this.pos,this.pos+=8)),devminor:r(this.INT8.subarray(this.pos,this.pos+=8)),prefix:n(this.INT8.subarray(this.pos,this.pos+=155))};this.pos+=12;e.offset=this.pos;return e};i.prototype.getMembers=function(){return this.headers};i.prototype.getContent=function(e){var t=this.headers[e];if(!t){return null}return this.INT8.subarray(t.offset,t.offset+t.size)};e.Tar=i;if(typeof define!=="undefined"&&define.amd){define(function(){return i})}else if(typeof module!=="undefined"&&module.exports){module.exports=i}})(typeof window !== "undefined" ? window : global);',
	$SYSC: {
		cb_context_t: Runtime.generateStructInfo([
			['void*', 'data'],
			['void*', 'cb']
		]),
		startup_data_t: Runtime.generateStructInfo([
			['b4096', 'gameName'],
			['i32', 'after']
		]),
		download_progress_data_t: Runtime.generateStructInfo([
			['i32', 'loaded'],
			['i32', 'total']
		]),
		download_complete_data_t: Runtime.generateStructInfo([
			['i32', 'progress']
		]),
		eula: 'LIMITED USE SOFTWARE LICENSE AGREEMENT\n\n \n\n' +
			'This Limited Use Software License Agreement (the "Agreement") is a legal agreement between you, the end-user, and Id Software, Inc. ("ID").  BY CONTINUING THE INSTALLATION OF THIS GAME DEMO PROGRAM ENTITLED QUAKE III: ARENA (THE "SOFTWARE"), BY LOADING OR RUNNING THE SOFTWARE, OR BY PLACING OR COPYING THE SOFTWARE ONTO YOUR COMPUTER HARD DRIVE, COMPUTER RAM OR OTHER STORAGE, YOU ARE AGREEING TO BE BOUND BY THE TERMS OF THIS AGREEMENT.\n\n\n\n' +
			'1.         Grant of License.  Subject to the terms and provisions of this Agreement, ID grants to you the non-exclusive and limited right to use the Software only in executable or object code form. The term "Software" includes all elements of the Software, including, without limitation, data files and screen displays.  You are not receiving any ownership or proprietary right, title or interest in or to the Software or the copyright, trademarks, or other rights related thereto.  For purposes of this section, "use" means loading the Software into RAM and/or onto computer hard drive, as well as installation of the Software on a hard disk or other storage device and means the uses permitted in section 3. hereinbelow.  You agree that the Software will not be shipped, transferred or exported into any country in violation of the U.S. Export Administration Act (or any other law governing such matters) by you or anyone at your direction and that you will not utilize and will not authorize anyone to utilize, in any other manner, the Software in violation of any applicable law.  The Software may not be downloaded or otherwise exported or exported into (or to a national or resident of) any country to which the U.S. has embargoed goods or to anyone or into any country who/which are prohibited, by applicable law, from receiving such property.\n\n\n\n' +
			'2.         Prohibitions. You, either directly or indirectly, shall not do any of the following acts:\n\n\n\n' +
			'a.         rent the Software;\n\n\n\n' +
			'b.         sell the Software;\n\n\n\n' +
			'c.         lease or lend the Software;\n\n\n\n' +
			'd.         offer the Software on a "pay-per-play" basis;\n\n\n\n' +
			'e.         distribute the Software (except as permitted by section 3. hereinbelow);\n\n\n\n' +
			'f.         in any other manner and through any medium whatsoever commercially exploit the Software or use the Software for any commercial purpose;\n\n\n\n' +
			'g.         disassemble, reverse engineer, decompile, modify or alter the Software including, without limitation, creating or developing extra or add-on levels for the Software;\n\n\n\n' +
			'h.         translate the Software;\n\n\n\n' +
			'i.         reproduce or copy the Software (except as permitted by section 3. hereinbelow);\n\n\n\n' +
			'j.         publicly display the Software;\n\n\n\n' +
			'k.         prepare or develop derivative works based upon the Software; or\n\n\n\n' +
			'l.         remove or alter any legal notices or other markings or legends, such as trademark and copyright notices, affixed on or within the Software.\n\n\n\n' +
			'3.         Permitted Distribution and Copying.  So long as this Agreement accompanies each copy you make of  the Software, and so long as you fully comply, at all times, with this Agreement, ID grants to you the non-exclusive and limited right to copy the Software and to distribute such copies of the Software free of charge for non-commercial purposes which shall include the free of charge distribution of copies of the Software as mounted on the covers of magazines; provided, however, you shall not copy or distribute the Software in any infringing manner or in any manner which violates any law or  third party right and you shall not distribute the Software together with any material which is  infringing, libelous, defamatory, obscene, false, misleading, or  otherwise illegal or unlawful. You agree to label conspicuously as "SHAREWARE" or "DEMO" each CD or other non-electronic copy of the Software that you make and distribute.  ID reserves all rights not granted in this Agreement. You shall not commercially distribute the Software  unless you first  enter into a separate contract with ID, a copy of which you may request, but which ID may decline to execute. For more information visit www.quake3arena.com.\n\n\n\n' +
			'4.         Intellectual Property Rights.  The Software and all copyrights, trademarks and all other conceivable intellectual property rights related to the Software are owned by ID and are protected by United States copyright laws, international treaty provisions and all applicable law, such as the Lanham Act.  You must treat the Software like any other copyrighted material, as required by 17 U.S.C., §101 et seq. and other applicable law. You agree to use your best efforts to see that any user of the Software licensed hereunder complies with this Agreement.  You agree that you are receiving a copy of the Software by license only and not by sale and that the "first sale" doctrine of 17 U.S.C. §109 does not apply to your receipt or use of the Software.\n\n\n\n' +
			'5.         NO WARRANTIES.  ID DISCLAIMS ALL WARRANTIES, WHETHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE WITH RESPECT TO THE SOFTWARE.  ID DOES NOT WARRANT THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR FREE OR THAT THE SOFTWARE WILL MEET YOUR SPECIFIC REQUIREMENTS.  ADDITIONAL STATEMENTS SUCH AS PRESENTATIONS, WHETHER ORAL OR WRITTEN, DO NOT CONSTITUTE WARRANTIES BY ID AND SHOULD NOT BE RELIED UPON. THIS SECTION 5. SHALL SURVIVE CANCELLATION OR TERMINATION OF THIS AGREEMENT.\n\n\n\n' +
			'6.         Governing Law, Venue, Indemnity and Liability Limitation.  This Agreement shall be construed in accordance with and governed by the applicable laws of the State of Texas and applicable United States federal law.  Copyright and other proprietary matters will be governed by United States laws and international treaties.  Exclusive venue for all litigation regarding this Agreement shall be in Dallas County, Texas and you agree to submit to the jurisdiction of the courts in Dallas, Texas for any such litigation. You agree to indemnify, defend and hold harmless ID and ID\'s officers, employees, directors, agents, licensees (excluding you), successors and assigns from and against all losses, lawsuits, damages, causes of action and claims relating to and/or arising from your breach of this Agreement.  You agree that your unauthorized use of the Software, or any part thereof, may immediately and irreparably damage ID such that ID could not be adequately compensated solely by a monetary award and that at ID\'s option ID shall be entitled to an injunctive order, in addition to all other available remedies including a monetary award, appropriately restraining and/or prohibiting such unauthorized use without the necessity of ID posting bond or other security. IN ANY CASE, ID AND ID\'S OFFICERS, EMPLOYEES, DIRECTORS, AGENTS, LICENSEES, SUBLICENSEES, SUCCESSORS AND ASSIGNS SHALL NOT BE LIABLE FOR LOSS OF DATA, LOSS OF PROFITS, LOST SAVINGS, SPECIAL, INCIDENTAL, CONSEQUENTIAL, INDIRECT, PUNITIVE OR OTHER SIMILAR DAMAGES ARISING FROM ANY ALLEGED CLAIM FOR BREACH OF WARRANTY, BREACH OF CONTRACT, NEGLIGENCE, STRICT PRODUCT LIABILITY, OR OTHER LEGAL THEORY EVEN IF ID OR ITS AGENT HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES OR EVEN IF SUCH DAMAGES ARE FORESEEABLE, OR LIABLE FOR ANY CLAIM BY ANY OTHER PARTY.  Some jurisdictions do not allow the exclusion or limitation of incidental or consequential damages, so the above limitation or exclusion may not apply to you. This Section 6. shall survive cancellation or termination of this Agreement.\n\n\n\n' +
			'7.         U.S. Government Restricted Rights. To the extent applicable, the United States Government shall only have those rights to use the Software as expressly stated and expressly limited and restricted in this Agreement, as provided in 48 C.F.R. §§ 227.7201 through 227.7204, inclusive.\n\n\n\n' +
			'8.         General Provisions.  Neither this Agreement nor any part or portion hereof shall be assigned or sublicensed by you.  ID may assign its rights under this Agreement in ID\'s sole discretion.  Should any provision of this Agreement be held to be void, invalid, unenforceable or illegal by a court of competent jurisdiction, the validity and enforceability of the other provisions shall not be affected thereby.  If any provision is determined to be unenforceable by a court of competent jurisdiction, you agree to a modification of such provision to provide for enforcement of the provision\'s intent, to the extent permitted by applicable law. Failure of ID to enforce any provision of this Agreement shall not constitute or be construed as a waiver of such provision or of the right to enforce such provision.  Immediately upon your failure to comply with or breach of any term or provision of this Agreement, THIS AGREEMENT AND YOUR LICENSE SHALL AUTOMATICALLY TERMINATE, WITHOUT NOTICE, AND ID MAY PURSUE ALL RELIEF AND REMEDIES AGAINST YOU WHICH ARE AVAILABLE UNDER APPLICABLE LAW AND/OR THIS AGREEMENT.   In the event this Agreement is terminated, you shall have no right to use the Software, in any manner, and you shall immediately destroy all copies of the Software in your possession, custody or control.\n\n\n\n' +
			'YOU ACKNOWLEDGE THAT YOU HAVE READ THIS AGREEMENT, YOU UNDERSTAND THIS AGREEMENT, AND UNDERSTAND THAT BY CONTINUING THE INSTALLATION OF THE SOFTWARE, BY LOADING OR RUNNING THE SOFTWARE, OR BY PLACING OR COPYING THE SOFTWARE ONTO YOUR COMPUTER HARD DRIVE OR RAM, YOU AGREE TO BE BOUND BY THE TERMS AND CONDITIONS OF THIS AGREEMENT.  YOU FURTHER AGREE THAT, EXCEPT FOR WRITTEN SEPARATE AGREEMENTS BETWEEN ID AND YOU, THIS AGREEMENT IS A COMPLETE AND EXCLUSIVE STATEMENT OF THE RIGHTS AND LIABILITIES OF THE PARTIES HERETO.  THIS AGREEMENT SUPERSEDES ALL PRIOR ORAL AGREEMENTS, PROPOSALS OR UNDERSTANDINGS, AND ANY OTHER COMMUNICATIONS BETWEEN ID AND YOU RELATING TO THE SUBJECT MATTER OF THIS AGREEMENT.',
		installers: [
			{
				name: 'linuxq3ademo-1.11-6.x86.gz.sh',
				offset: 5468,
				paks: [
					{ src: 'demoq3/pak0.pk3', dest: 'baseq3/pak0.pk3', checksum: 2483777038 }
				]
			},
			{
				name: 'linuxq3apoint-1.32b-3.x86.run',
				offset: 8251,
				paks: [
					{ src: 'baseq3/pak1.pk3', dest: 'baseq3/pak1.pk3', checksum: 1635885364 },
					{ src: 'baseq3/pak2.pk3', dest: 'baseq3/pak2.pk3', checksum: 2142044321 },
					{ src: 'baseq3/pak3.pk3', dest: 'baseq3/pak3.pk3', checksum: 682311620 },
					{ src: 'baseq3/pak4.pk3', dest: 'baseq3/pak4.pk3', checksum: 4113726565 },
					{ src: 'baseq3/pak5.pk3', dest: 'baseq3/pak5.pk3', checksum: 590466266 },
					{ src: 'baseq3/pak6.pk3', dest: 'baseq3/pak6.pk3', checksum: 231612509 },
					{ src: 'baseq3/pak7.pk3', dest: 'baseq3/pak7.pk3', checksum: 3663817674 },
					{ src: 'baseq3/pak8.pk3', dest: 'baseq3/pak8.pk3', checksum: 136401958 }
				]
			}
		],
		manifest: null,
		Print: function (str) {
			str = allocate(intArrayFromString(str + '\n'), 'i8', ALLOC_STACK);

			_Com_Printf(str);
		},
		Error: function (level, err) {
			if (level === 'fatal') {
				level = 0;
			} else if (level === 'drop') {
				level = 1;
			} else if (level === 'serverdisconnect') {
				level = 2;
			} else if (level === 'disconnect') {
				level = 3;
			} else if (level === 'need_cd') {
				level = 4;
			} else {
				level = 0;
			}

			err = allocate(intArrayFromString(err + '\n'), 'i8', ALLOC_STACK);

			_Com_Error(level, err);
		},
    Con_ToggleConsole_f: function () {
      _Con_ToggleConsole_f()
      //_trap_SendClientCommand(allocate(intArrayFromString("/god"), "i8", ALLOC_STACK))
    },
		ProxyCallback: function (context) {
			try {
				_Com_ProxyCallback(context);
			} catch (e) {
				if (e instanceof ExitStatus) {
					return;
				}
				// TODO should we try and call back in using __Error?
				throw e;
			}
		},
		CRC32File: function (path) {
			var stack = Runtime.stackSave();
			var chunkSize = 1024*1024;
			var bufp = allocate(chunkSize, 'i8', ALLOC_STACK);
			var crc = CRC32.Start();

			var start = Date.now();

			try {
				var slab = {{{ makeGetSlabs('bufp', 'i8', true) }}};
				var n = 0;
				var pos = 0;
				var stream = FS.open(path, 'r', 0666);
				do {
					n = FS.read(stream, slab, bufp, chunkSize, pos);
					crc = CRC32.Update(crc, slab, bufp, n);
					pos += n;
				} while (n);
				FS.close(stream);
			} catch (e) {
				Runtime.stackRestore(stack);
				return null;
			}

			SYSC.Print('generated crc32 for ' + path + ' in ' + ((Date.now() - start) / 1000).toFixed(2) + ' seconds');

			Runtime.stackRestore(stack);
			return CRC32.Finish(crc);
		},
		GetCDN: function () {
			return Pointer_stringify(_Com_GetCDN());
		},
		GetManifest: function () {
			var manifest = Pointer_stringify(_Com_GetManifest());

			if (!manifest) {
				return [];
			}

			return manifest.split(' ').map(function (entry) {
				var split = entry.split('@');

				return {
					name: split[0],
					checksum: parseInt(split[1], 10),
					compressed: parseInt(split[2], 10)
				};
			});
		},
		DownloadAsset: function (asset, onprogress, onload) {
			// The quake server itself uses a different fs_cdn than the client...
			// so when it tells us where to download assets from, replace the 
			// hostname with the browser's url hostname.
			var root = SYSC.GetCDN();
			var server = root.replace(/^[^:]+/, document.location.hostname);
			var name = asset.name.replace(/(.+\/|)(.+?)$/, '$1' + asset.checksum + '-$2');
			console.log(`DownloadAsset from ${server} (originally, ${root}) - ${name}`);

			var url = 'http://' + server + '/assets/' + name;

			SYS.DoXHR(url, {
				dataType: 'arraybuffer',
				onprogress: onprogress,
				onload: onload
			});
		},
		DownloadAssets: function (assets, onstartasset, onprogress, onendasset, callback) {
			var progress = [];

			function downloadedBytes() {
				return progress.reduce(function (a, b) { return a + b; });
			}

			function totalBytes() {
				return assets.reduce(function (a, b) { return a + b.compressed; }, 0);
			}

			function nextDownload() {
				nextDownload.pos = nextDownload.pos == undefined ? 0 : nextDownload.pos + 1;

				if (nextDownload.pos >= assets.length) {
					return callback();
				}

				var asset = assets[nextDownload.pos];

				onstartasset(asset);

				SYSC.DownloadAsset(asset, function (loaded, total) {
					progress[nextDownload.pos] = loaded;

					onprogress(downloadedBytes(), totalBytes());
				}, function (err, data) {
					if (err) return callback(err);

					onendasset(asset, data, function (err) {
						if (err) return callback(err);

						setTimeout(nextDownload);
					});
				});
			}

			nextDownload();
		},
		UpdateManifest: function (callback) {
			var fs_cdn = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('fs_cdn'), 'i8', ALLOC_STACK)));
			var fs_game = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('fs_game'), 'i8', ALLOC_STACK)));
			var com_basegame = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('com_basegame'), 'i8', ALLOC_STACK)));
			var mapname = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('mapname'), 'i8', ALLOC_STACK)));
			var url = 'http://' + fs_cdn + '/assets/manifest.json';

			function isInstaller(name) {
				return SYSC.installers.some(function (installer) {
					return installer.name === name;
				});
			}

			function isCommon(name) {
				var basepakRx = RegExp('(' + com_basegame + (fs_game ? '|' + fs_game : '') + ')\/pak.+\.pk3$');
				return name.match(basepakRx);
			}

			function isMapPak(name) {
				return PATH.basename(name) === (mapname + '.pk3');
			}

			function activePaks(entry) {
				return isInstaller(entry.name) || isCommon(entry.name) || isMapPak(entry.name);
			}

			function formatManifestString(manifest) {
				return manifest.map(function (entry) {
					return entry.name + '@' + entry.checksum + '@' + entry.compressed;
				}).join(' ');
			}

			SYS.DoXHR(url, {
				dataType: 'json',
				onload: function (err, manifest) {
					if (err) return callback(new Error('Failed to download and parse manifest, ' + err.message));

					var fs_manifestName = allocate(intArrayFromString('fs_manifest'), 'i8', ALLOC_STACK);
					var fs_manifest = allocate(intArrayFromString(formatManifestString(manifest.filter(activePaks))), 'i8', ALLOC_STACK);
					_Cvar_Set(fs_manifestName, fs_manifest);

					var fs_completeManifestName = allocate(intArrayFromString('fs_completeManifest'), 'i8', ALLOC_STACK);
					var fs_completeManifest = allocate(intArrayFromString(formatManifestString(manifest)), 'i8', ALLOC_STACK);
					_Cvar_Set(fs_completeManifestName, fs_completeManifest);

					return callback();
				}
			});
		},
		SavePak: function (name, buffer, callback) {
			var fs_homepath = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('fs_homepath'), 'i8', ALLOC_STACK)));
			var localPath = PATH.join(fs_homepath, name);

			try {
				FS.mkdir(PATH.dirname(localPath), 0777);
			} catch (e) {
				if (e.errno !== ERRNO_CODES.EEXIST) {
					return callback(e);
				}
			}

			FS.writeFile(localPath, new Uint8Array(buffer), { encoding: 'binary', flags: 'w', canOwn: true });

			FS.syncfs(callback);
		},
		ValidateInstaller: function (installer) {
			var fs_homepath = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('fs_homepath'), 'i8', ALLOC_STACK)));

			for (var i = 0; i < installer.paks.length; i++) {
				var pak = installer.paks[i];
				var localPath = PATH.join(fs_homepath, pak.dest);
				var crc = SYSC.CRC32File(localPath);

				if (crc !== pak.checksum) {
					return false;
				}
			}

			return true;
		},
		DirtyInstallers: function () {
			var installers = [];
			var assets = SYSC.GetManifest();

			for (var i = 0; i < SYSC.installers.length; i++) {
				var installer = SYSC.installers[i];

				var asset;
				for (var j = 0; j < assets.length; j++) {
					if (assets[j].name === installer.name) {
						asset = assets[j];
						break;
					}
				}

				if (!asset) {
					return callback(new Error('Failed to find "' + installer.name + '" in manifest'));
				}

				if (!SYSC.ValidateInstaller(installer)) {
					// append the installer info to the asset
					asset.installer = installer;

					installers.push(asset);
				}
			}

			return installers;
		},
		ExtractInstaller: function (data, paks, callback) {
			var gunzip = new Zlib.Gunzip(data);
			var buffer = gunzip.decompress();
			var tar = new Tar(buffer);

			function nextEntry() {
				nextEntry.pos = nextEntry.pos == undefined ? 0 : nextEntry.pos + 1;

				if (nextEntry.pos >= paks.length) {
					return callback();
				}

				var entry = paks[nextEntry.pos];

				var pakPath = entry.src;
				var buffer = tar.getContent(pakPath);

				// TODO validate buffer checksum

				SYSC.SavePak(entry.dest, buffer, function (err) {
					if (err) return callback(err);

					nextEntry();
				});
			}

			nextEntry();
		},
		SyncInstallers: function (callback) {
			var downloads = SYSC.DirtyInstallers();

			if (!downloads.length) {
				return callback();
			}

			SYS.PromptEULA(function (err) {
				if (err) return callback(err);

				SYSC.DownloadAssets(downloads, function (asset) {
					SYS.LoadingDescription('loading ' + asset.name);
				}, function (loaded, total) {
					SYS.LoadingProgress(loaded / total);
				}, function (asset, data, next) {
					SYSC.ExtractInstaller(new Uint8Array(data, asset.installer.offset), asset.installer.paks, next);
				}, function (err) {
					SYS.LoadingDescription(null);

					setTimeout(function () {
						callback(err);
					});
				});
			});
		},
		ValidatePak: function (asset) {
			var fs_homepath = Pointer_stringify(_Cvar_VariableString(allocate(intArrayFromString('fs_homepath'), 'i8', ALLOC_STACK)));
			var localPath = PATH.join(fs_homepath, asset.name);
			var crc = SYSC.CRC32File(localPath);

			return crc === asset.checksum;
		},
		DirtyPaks: function () {
			return SYSC.GetManifest().filter(function (asset) {
				return asset.name.indexOf('.pk3') !== -1 && !SYSC.ValidatePak(asset);
			});
		},
		SyncPaks: function (callback) {
			var downloads = SYSC.DirtyPaks();

			SYSC.DownloadAssets(downloads, function (asset) {
				SYS.LoadingDescription('loading ' + asset.name);
			}, function (loaded, total) {
				SYS.LoadingProgress(loaded / total);
			}, function (asset, data, next) {
				SYSC.SavePak(asset.name, data, next);
			}, function (err) {
				SYS.LoadingDescription(null);

				setTimeout(function () {
					callback(err);
				});
			});
		},
		FS_Startup: function (callback) {
			SYSC.UpdateManifest(function (err) {
				if (err) return callback(err);

				SYSC.SyncInstallers(function (err) {
					if (err) return callback(err);

					SYSC.SyncPaks(Browser.safeCallback(callback));
				});
			});
		},
		FS_Shutdown: function (callback) {
			callback(null);
		},
	},
	Sys_DefaultHomePath: function () {
		return 0;
	},
	Sys_RandomBytes: function (string, len) {
		return false;
	},
	Sys_GetClipboardData: function () {
		return 0;
	},
	Sys_LowPhysicalMemory: function () {
		return false;
	},
	Sys_Basename__deps: ['$PATH'],
	Sys_Basename: function (path) {
		path = Pointer_stringify(path);
		path = PATH.basename(path);
		var basename = allocate(intArrayFromString(path), 'i8', ALLOC_STACK);
		return basename;
	},
	Sys_Dirname__deps: ['$PATH'],
	Sys_Dirname: function (path) {
		path = Pointer_stringify(path);
		path = PATH.dirname(path);
		var dirname = allocate(intArrayFromString(path), 'i8', ALLOC_STACK);
		return dirname;
	},
	Sys_Mkfifo: function (path) {
		return 0;
	},
	Sys_ListFiles__deps: ['$PATH', 'Z_Malloc', 'S_Malloc'],
	Sys_ListFiles: function (directory, ext, filter, numfiles, dironly) {
		directory = Pointer_stringify(directory);
		ext = Pointer_stringify(ext);
		if (ext === '/') {
			ext = null;
			dironly = true;
		}

		// TODO support filter
		
		var contents;
		try {
			contents = FS.readdir(directory);
		} catch (e) {
			{{{ makeSetValue('numfiles', '0', '0', 'i32') }}};
			return null;
		}

		var matches = [];
		for (var i = 0; i < contents.length; i++) {
			var name = contents[i];
			var stat = FS.stat(PATH.join(directory, name));

			if (dironly && !FS.isDir(stat.mode)) {
				continue;
			}

			if ((!ext || name.lastIndexOf(ext) === (name.length - ext.length))) {
				matches.push(name);
			}
		}

		{{{ makeSetValue('numfiles', '0', 'matches.length', 'i32') }}};

		if (!matches.length) {
			return null;
		}

		// return a copy of the match list
		var list = _Z_Malloc((matches.length + 1) * 4);

		var i;
		for (i = 0; i < matches.length; i++) {
			var filename = _S_Malloc(matches[i].length + 1);

			writeStringToMemory(matches[i], filename);

			// write the string's pointer back to the main array
			{{{ makeSetValue('list', 'i*4', 'filename', 'i32') }}};
		}

		// add a NULL terminator to the list
		{{{ makeSetValue('list', 'i*4', '0', 'i32') }}};

		return list;
	},
	Sys_FreeFileList__deps: ['Z_Free'],
	Sys_FreeFileList: function (list) {
		if (!list) {
			return;
		}

		var ptr;

		for (var i = 0; (ptr = {{{ makeGetValue('list', 'i*4', 'i32') }}}); i++) {
			_Z_Free(ptr);
		}

		_Z_Free(list);
	},
	Sys_FOpen: function (ospath, mode) {
		return _fopen(ospath, mode);
	},
	Sys_Mkdir: function (directory) {
		directory = Pointer_stringify(directory);
		try {
			FS.mkdir(directory, 0777);
		} catch (e) {
			if (!(e instanceof FS.ErrnoError)) {
				SYSC.Error('drop', e.message);
			}
			return e.errno === ERRNO_CODES.EEXIST;
		}
		return true;
	},
	Sys_Cwd: function () {
		var cwd = allocate(intArrayFromString(FS.cwd()), 'i8', ALLOC_STACK);
		return cwd;
	},
	Sys_Sleep: function () {
	},
	Sys_SetEnv: function (name, value) {
		name = Pointer_stringify(name);
		value = Pointer_stringify(value);
	},
	Sys_PID: function () {
		return 0;
	},
	Sys_PIDIsRunning: function (pid) {
		return 1;
	}
};

autoAddDeps(LibrarySysCommon, '$SYSC');
mergeInto(LibraryManager.library, LibrarySysCommon);
