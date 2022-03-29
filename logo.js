var xmlns = "http://www.w3.org/2000/svg";

pipe = function(x, y, height, width, mouth){
	var polygon = document.createElementNS(xmlns, "polygon");
	var mwidth=height/60
	polygon.setAttribute("style","fill:black;stroke:green;")
	var points=(x+width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth-height).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth-height).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth).toString()
	points+=' '+x.toString()+','+y.toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth-mwidth).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth-mwidth).toString()
	polygon.setAttribute("points",points); 
	return polygon
}
plateface = function(nbPipes, x, y, firstH, lastH, width, firstM, lastM, symetric){
	var deltaH=(lastH-firstH)/(nbPipes-1)
	var deltaM=(lastM-firstM)/(nbPipes-1)
	if(symetric){deltaH*=2; deltaM*=2;}
	var h=firstH
	var m=firstM
	var padding=0
	for(let p=0; p<nbPipes; p++){
		if(symetric && p == ~~(nbPipes/2)){
			deltaH*=-1
			deltaM*=-1
		}
		document.getElementById('logo').appendChild(pipe(x+p*(width+padding), y, h, width, m))
		h+=deltaH
		m+=deltaM
	}
	return x+width/2+(nbPipes-1)*(width+padding)
}
logo = function(x0, y0, scale){
	var s=scale
	var nA=3, nB=5, nC=10, nD=9
	var dy=7*s, p=4.5*s, wA=5*s, wB=3*s, wC=3*s, wD=2*s
	var x1=plateface(nA, x0, y0-dy, s*77, s*88, wA, s*17, s*17, true)+p
	var x2=plateface(nB, x1, y0-dy, s*68, s*42, wB, s*13, s*17, false)+p
	var x3=plateface(nC, x2, y0, s*42, s*30, wC, s*11, s*11, false)+p
	var x4=plateface(nD, x3, y0, s*32, s*41, wD, s*11, s*6, true)+p
	var x5=plateface(nC, x4, y0, s*30, s*42, wC, s*11, s*11, false)+p
	var x6=plateface(nB, x5, y0-dy, s*42, s*68, wB, s*17, s*13, false)+p
	var x7=plateface(nA, x6, y0-dy, s*77, s*88, wA, s*17, s*17, true)+p
}


var xmlns="http://www.w3.org/2000/svg";pipe=function(t,e,n,r,o){var i=document.createElementNS(xmlns,"polygon"),a=n/60;i.setAttribute("style","fill:black;stroke:green;");var g=(t+r/2).toString()+","+(e-o).toString();return g+=" "+(t-r/2).toString()+","+(e-o).toString(),g+=" "+(t-r/2).toString()+","+(e-o-n).toString(),g+=" "+(t+r/2).toString()+","+(e-o-n).toString(),g+=" "+(t+r/2).toString()+","+(e-o).toString(),g+=" "+t.toString()+","+e.toString(),g+=" "+(t-r/2).toString()+","+(e-o).toString(),g+=" "+(t-r/2).toString()+","+(e-o-a).toString(),g+=" "+(t+r/2).toString()+","+(e-o-a).toString(),i.setAttribute("points",g),i},plateface=function(e,n,r,t,o,i,a,g,l){var S=(o-t)/(e-1),p=(g-a)/(e-1);l&&(S*=2,p*=2);var c=t,f=a;for(let t=0;t<e;t++)l&&t==~~(e/2)&&(S*=-1,p*=-1),document.getElementById("logo").appendChild(pipe(n+t*(i+0),r,c,i,f)),c+=S,f+=p;return n+i/2+(e-1)*(i+0)},logo=function(t,e,n){var r=n,o=7*r,i=4.5*r,a=5*r,g=3*r,l=3*r,n=2*r,t=plateface(3,t,e-o,77*r,88*r,a,17*r,17*r,!0)+i,t=plateface(5,t,e-o,68*r,42*r,g,13*r,17*r,!1)+i,t=plateface(10,t,e,42*r,30*r,l,11*r,11*r,!1)+i,n=plateface(9,t,e,32*r,41*r,n,11*r,6*r,!0)+i,l=plateface(10,n,e,30*r,42*r,l,11*r,11*r,!1)+i,g=plateface(5,l,e-o,42*r,68*r,g,17*r,13*r,!1)+i;plateface(3,g,e-o,77*r,88*r,a,17*r,17*r,!0)};
