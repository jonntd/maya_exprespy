//Maya ASCII 2012 scene
//Name: noiseDeformer.ma
//Codeset: 932
requires maya "2012";
requires "exprespy" "2.0.0.20161029";
currentUnit -l centimeter -a degree -t ntsc;
createNode transform -n "pSphere1";
	addAttr -ci true -sn "octave" -ln "octave" -dv 3 -smn 1 -smx 5 -at "long";
	addAttr -ci true -sn "frequency" -ln "frequency" -dv 0.2 -smn 0 -smx 1 -at "double";
	addAttr -ci true -sn "amplitude" -ln "amplitude" -dv 2 -smn 0 -smx 5 -at "double";
	addAttr -ci true -sn "timeFrequency" -ln "timeFrequency" -dv 1 -smn 0 -smx 5 -at "double";
	addAttr -ci true -sn "worldSpace" -ln "worldSpace" -min 0 -max 1 -at "bool";
	setAttr -cb on ".octave";
	setAttr -cb on ".frequency";
	setAttr -cb on ".amplitude";
	setAttr -cb on ".timeFrequency";
	setAttr -cb on ".worldSpace";
createNode mesh -n "pSphereShape1Orig" -p "pSphere1";
	setAttr -k off ".v";
	setAttr ".io" yes;
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
createNode mesh -n "pSphereShape1" -p "pSphere1";
	setAttr -k off ".v";
	setAttr ".vir" yes;
	setAttr ".vif" yes;
	setAttr ".uvst[0].uvsn" -type "string" "map1";
	setAttr ".cuvs" -type "string" "map1";
	setAttr ".dcc" -type "string" "Ambient+Diffuse";
	setAttr ".covm[0]"  0 1 1;
	setAttr ".cdvm[0]"  0 1 1;
createNode polySphere -n "polySphere1";
	setAttr ".r" 10;
	setAttr ".sa" 40;
	setAttr ".sh" 40;
createNode exprespy -n "exprNoiseDeformer";
	setAttr ".cd" -type "string" (
		"if not COUNT:\n    from maya.api.OpenMaya import MFnMesh, MVector, MPoint\n    kWorld = api.MSpace.kWorld\n    kObject = api.MSpace.kObject\n\n    from math import floor, fmod\n    import random\n\n    def _random_permutaion(n, seed=1):\n        st = random.getstate()\n        random.seed(seed)\n        res = range(n)\n        random.shuffle(res)\n        random.setstate(st)\n        return res\n\n    _PERM = _random_permutaion(256) * 2\n\n    _GRAD3 = (\n        (1,1,0),(-1,1,0),(1,-1,0),(-1,-1,0),\n        (1,0,1),(-1,0,1),(1,0,-1),(-1,0,-1),\n        (0,1,1),(0,-1,1),(0,1,-1),(0,-1,-1),\n        (1,1,0),(0,-1,1),(-1,1,0),(0,-1,-1),\n    )\n\n    def _grad3(hash, x, y, z):\n        g = _GRAD3[hash % 16]\n        return x * g[0] + y * g[1] + z * g[2]\n\n    _lerp = lambda t, a, b: a + t * (b - a)\n    _fade = lambda t: t * t * t * (t * (t * 6 - 15) + 10)\n\n    def noise3(x, y, z):\n        i = floor(x)\n        j = floor(y)\n        k = floor(z)\n        x -= i\n        y -= j\n        z -= k\n        i = int(fmod(i, 256))\n        j = int(fmod(j, 256))\n"
		+ "        k = int(fmod(k, 256))\n        u = _fade(x)\n        v = _fade(y)\n        w = _fade(z)\n\n        ii = (i + 1) % 256\n        jj = (j + 1) % 256\n        kk = (k + 1) % 256\n\n        A = _PERM[i]\n        AA = _PERM[A + j]\n        AB = _PERM[A + jj]\n        B = _PERM[ii]\n        BA = _PERM[B + j]\n        BB = _PERM[B + jj]\n\n        return _lerp(w, _lerp(v, _lerp(u, _grad3(_PERM[AA + k], x, y, z),\n                                          _grad3(_PERM[BA + k], x - 1, y, z)),\n                                 _lerp(u, _grad3(_PERM[AB + k], x, y - 1, z),\n                                          _grad3(_PERM[BB + k], x - 1, y - 1, z))),\n                        _lerp(v, _lerp(u, _grad3(_PERM[AA + kk], x, y, z - 1),\n                                          _grad3(_PERM[BA + kk], x - 1, y, z - 1)),\n                                 _lerp(u, _grad3(_PERM[AB + kk], x, y - 1, z - 1),\n                                          _grad3(_PERM[BB + kk], x - 1, y - 1, z - 1))))\n\n    def noise3p(pnt, octave=1):\n        pnt = MPoint(pnt)\n"
		+ "        noise = 0.\n        amplitude = 1.\n        maxvalue = 0.\n        frequency = 1.\n        for i in range(octave):\n            pnt *= frequency\n            noise += amplitude * noise3(pnt.x, pnt.y, pnt.z)\n            maxvalue += amplitude\n            amplitude *= .5\n            frequency *= 2.\n        return noise / maxvalue\n\ndata = IN[0]\nt = IN[1]\noctave = IN[10]\nfrequency = IN[11]\namplitude = IN[12]\nt *= IN[13]\nspace = kWorld if IN[14] else kObject\n\noffset = MVector(t, t, t)\nfn = MFnMesh(data)\npnts = fn.getPoints(space)\nnrms = fn.getVertexNormals(True, space)\nfor i, nrm in enumerate(nrms):\n    pnt = pnts[i]\n    pnt *= frequency\n    pnt += offset\n    pnts[i] += MVector(nrm) * (noise3p(pnt, octave) * amplitude)\nfn.setPoints(pnts, space)\nOUT[0] = data\n");
	setAttr -s 7 ".i";
connectAttr "polySphere1.out" "pSphereShape1Orig.i";
connectAttr "exprNoiseDeformer.o[0]" "pSphereShape1.i";
connectAttr "pSphereShape1Orig.w" "exprNoiseDeformer.i[0]";
connectAttr ":time1.o" "exprNoiseDeformer.i[1]";
connectAttr "pSphere1.octave" "exprNoiseDeformer.i[10]";
connectAttr "pSphere1.frequency" "exprNoiseDeformer.i[11]";
connectAttr "pSphere1.amplitude" "exprNoiseDeformer.i[12]";
connectAttr "pSphere1.timeFrequency" "exprNoiseDeformer.i[13]";
connectAttr "pSphere1.worldSpace" "exprNoiseDeformer.i[14]";
connectAttr "pSphereShape1Orig.iog" ":initialShadingGroup.dsm" -na;
connectAttr "pSphereShape1.iog" ":initialShadingGroup.dsm" -na;
// End of noiseDeformer.ma
