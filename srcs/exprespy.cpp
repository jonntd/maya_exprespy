#include <Python.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MDataBlock.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>
#include <maya/MFnDependencyNode.h>

#ifdef _WIN32
#pragma warning(disable: 4127)
#endif

#define EXPRESPY_NODE_ID  0x00070004  // ���������ƏՓ˂���Ȃ珑�������邱�ƁB(Free: 0x00000000 �` 0x0007ffff)


//=============================================================================
/// Functions.
//=============================================================================
/// ���W���[�����C���|�[�g����BPyImport_ImportModule ��荂�����B
static inline PyObject* _importModule(const char* name)
{
    PyObject* nameo = PyString_FromString(name);
    if (nameo) {
        PyObject* mod = PyImport_Import(nameo);
        Py_DECREF(nameo);
        return mod;
    }
    return NULL;
}

/// dict ���� type �I�u�W�F�N�g�𓾂�B
static inline PyObject* _getTypeObject(PyObject* dict, const char* name)
{
    PyObject* o = PyDict_GetItemString(dict, name);
    return PyType_Check(o) ? o : NULL;
}

/// �l�̎Q�Ƃ�D���� dict �ɒl���Z�b�g����B
static inline void _setDictStealValue(PyObject* dic, const char* key, PyObject* valo)
{
    if (valo) {
        PyDict_SetItemString(dic, key, valo);
        Py_DECREF(valo);
    } else {
        PyDict_SetItemString(dic, key, Py_None);
    }
}

/// �L�[�ƒl�̎Q�Ƃ�D���� I/O �p dict �ɒl���Z�b�g����B
static inline void _setIODictStealValue(PyObject* dic, PyObject* keyo, PyObject* valo)
{
    if (valo) {
        PyDict_SetItem(dic, keyo, valo);
        Py_DECREF(valo);
    } else {
        PyDict_SetItem(dic, keyo, Py_None);
    }
    Py_DECREF(keyo);
}

/// dict �� int ���L�[�Ƃ��� PyObject* ���Z�b�g����B
static inline void _setDictValue(PyObject* dic, int key, PyObject* valo)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        PyDict_SetItem(dic, keyo, valo);
        Py_DECREF(keyo);
    }
}

/// dict ���� int ���L�[�Ƃ��� PyObject* �𓾂�B
static inline PyObject* _getDictValue(PyObject* dic, int key)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        PyObject* valo = PyDict_GetItem(dic, keyo);
        Py_DECREF(keyo);
        return valo;
    }
    return NULL;
}

/// �^���ʍς݂� sequence �� PyObject* ���� double3 �� data �𓾂�B
static inline MObject _getDouble3Value(PyObject* valo)
{
    MFnNumericData fnOutData;
    MObject data = fnOutData.create(MFnNumericData::k3Double);
    fnOutData.setData(
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 0)),
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 1)),
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 2))
    );
    return data;
}

/// �^���ʍς݂� sequence �� PyObject* ���� double4 �� data �𓾂�B
static inline MObject _getDouble4Value(PyObject* valo)
{
    MFnNumericData fnOutData;
    MObject data = fnOutData.create(MFnNumericData::k3Double);
    fnOutData.setData(
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 0)),
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 1)),
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 2)),
        PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 3))
    );
    return data;
}

/// �^���ʍς݂� sequence �� PyObject* ���� float3 �� data �𓾂�B
static inline MObject _getFloat3Value(PyObject* valo)
{
    MFnNumericData fnOutData;
    MObject data = fnOutData.create(MFnNumericData::k3Float);
    fnOutData.setData(
        static_cast<float>(PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 0))),
        static_cast<float>(PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 1))),
        static_cast<float>(PyFloat_AS_DOUBLE(PySequence_GetItem(valo, 2)))
    );
    return data;
}

/// sequence �� PyObject* ���� double2, double3, double4, long2, long3 �̂����ꂩ�𓾂�B
static inline MObject _getTypedSequence(PyObject* valo)
{
    MObject data;

    Py_ssize_t num = PySequence_Length(valo);
    if (num < 2 || 4 < num) return data;

    bool hasFloat = false;
    int ival[4];
    double fval[4];
    for (Py_ssize_t i=0; i<num; ++i) {
        PyObject* v = PySequence_GetItem(valo, i);
        if (PyFloat_Check(v)) {
            hasFloat = true;
            fval[i] = PyFloat_AS_DOUBLE(v);
        } else if (PyInt_Check(v)) {
            ival[i] = PyInt_AS_LONG(v);
            fval[i] = static_cast<double>(ival[i]);
        } else {
            return data;
        }
    }

    MFnNumericData fnOutData;
    switch (num) {
    case 2:
        if (hasFloat) {
            data = fnOutData.create(MFnNumericData::k2Double);
            fnOutData.setData(fval[0], fval[1]);
        } else {
            data = fnOutData.create(MFnNumericData::k2Long);
            fnOutData.setData(ival[0], ival[1]);
        }
        break;
    case 3:
        if (hasFloat) {
            data = fnOutData.create(MFnNumericData::k3Double);
            fnOutData.setData(fval[0], fval[1], fval[2]);
        } else {
            data = fnOutData.create(MFnNumericData::k3Long);
            fnOutData.setData(ival[0], ival[1], ival[2]);
        }
        break;
    case 4:
        data = fnOutData.create(MFnNumericData::k4Double);
        fnOutData.setData(fval[0], fval[1], fval[2], fval[3]);
        break;
    }
    return data;
}

/// �^���ʍς݂� PyObject* �̒l�� MMatrix �ɓ���B
static inline void _getMatrixValueFromPyObject(MMatrix& mat, PyObject* valo)
{
    PyObject* getElement = PyObject_GetAttrString(valo, "getElement");
    if (getElement) {
        PyObject* idxs[] = {
            PyInt_FromLong(0), PyInt_FromLong(1), PyInt_FromLong(2), PyInt_FromLong(3)
        };
        PyObject* args = PyTuple_New(2);
        for (unsigned i=0; i<4; ++i) {
            PyTuple_SET_ITEM(args, 0, idxs[i]);
            for (unsigned j=0; j<4; ++j) {
                PyTuple_SET_ITEM(args, 1, idxs[j]);

                PyObject* vo = PyObject_CallObject(getElement, args);
                //PyObject* vo = PyObject_CallFunction(getElement, "(ii)", i, j);
                if (vo) {
                    mat.matrix[i][j] = PyFloat_AS_DOUBLE(vo);
                    Py_DECREF(vo);
                } else {
                    mat.matrix[i][j] = MMatrix::identity[i][j];
                }
            }
        }

        PyTuple_SET_ITEM(args, 0, idxs[0]);
        PyTuple_SET_ITEM(args, 1, idxs[1]);
        Py_DECREF(args);
        Py_DECREF(idxs[2]);
        Py_DECREF(idxs[3]);

    } else {
        mat.setToIdentity();
    }
}


//=============================================================================
/// Exprespy node class.
//=============================================================================
class Exprespy : public MPxNode {
    PyObject* _base_globals;
    PyObject* _input;
    PyObject* _output;
    PyObject* _globals;
    PyCodeObject* _codeobj;
    PyObject* _mplug2_input;
    PyObject* _mplug2_output;
    PyObject* _mplug1_input;
    PyObject* _mplug1_output;

    PyObject* _api2_dict;
    PyObject* _type2_MVector;
    PyObject* _type2_MPoint;
    PyObject* _type2_MEulerRotation;
    PyObject* _type2_MMatrix;
    PyObject* _type2_MObject;
    PyObject* _type2_MQuaternion;
    PyObject* _type2_MColor;
    PyObject* _type2_MFloatVector;
    PyObject* _type2_MFloatPoint;

    PyObject* _api1_dict;
    PyObject* _type1_MObject;

    PyObject* _type_StringIO;
    PyObject* _sys_dict;
    PyObject* _sys_stdout;
    PyObject* _sys_stderr;

    int _count;

    MStatus _compileCode(MDataBlock&);
    MStatus _executeCode(MDataBlock&);

    void _printPythonError();

    void _setInputScalar(int key, const double& val);
    void _setInputString(int key, const MString& str);
    void _setInputShort2(int key, MObject& data);
    void _setInputShort3(int key, MObject& data);
    void _setInputLong2(int key, MObject& data);
    void _setInputLong3(int key, MObject& data);
    void _setInputFloat2(int key, MObject& data);
    void _setInputFloat3(int key, MObject& data);
    void _setInputDouble2(int key, MObject& data);
    void _setInputDouble3(int key, MObject& data);
    void _setInputDouble4(int key, MObject& data);
    void _setInputVector3(int key, MObject& data);
    void _setInputMatrix(int key, MObject& data);

    void _setInputMObject2(int key);
    void _setOutputMObject2(int key, PyObject* valo);
    void _preparePyPlug2();

    void _setInputMObject1(int key);
    void _setOutputMObject1(int key, PyObject* valo);
    void _preparePyPlug1();

public:
    static MTypeId id;
    static MString name;

    static MObject aCode;
    static MObject aInputsAsApi1Object;
    static MObject aCompiled;
    static MObject aInput;
    static MObject aOutput;

    static void* creator() { return new Exprespy; }
    static MStatus initialize();

    Exprespy();
    virtual ~Exprespy();

    MStatus compute(const MPlug&, MDataBlock&);
};

MTypeId Exprespy::id(EXPRESPY_NODE_ID);
MString Exprespy::name("exprespy");
MObject Exprespy::aCode;
MObject Exprespy::aInputsAsApi1Object;
MObject Exprespy::aCompiled;
MObject Exprespy::aInput;
MObject Exprespy::aOutput;


//------------------------------------------------------------------------------
/// Static: Initialization.
//------------------------------------------------------------------------------
MStatus Exprespy::initialize()
{
    MFnTypedAttribute fnTyped;
    MFnNumericAttribute fnNumeric;
    MFnGenericAttribute fnGeneric;

    // code
    aCode = fnTyped.create("code", "cd", MFnData::kString);
    fnTyped.setConnectable(false);
    CHECK_MSTATUS_AND_RETURN_IT( addAttribute(aCode) );

    // inputsAsApi1Object
    aInputsAsApi1Object = fnNumeric.create("inputsAsApi1Object", "iaa1o", MFnNumericData::kBoolean, false);
    fnTyped.setConnectable(false);
    CHECK_MSTATUS_AND_RETURN_IT( addAttribute(aInputsAsApi1Object) );

    // compiled
    aCompiled = fnNumeric.create("compile", "cm", MFnNumericData::kBoolean, false);
    fnNumeric.setWritable(false);
    fnNumeric.setStorable(false);
    fnNumeric.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT( addAttribute(aCompiled) );

    // input
    aInput = fnGeneric.create("input", "i");
    // ���͂̏ꍇ�� kAny �����ŃR�l�N�g�͑S�ċ��e����邪 setAttr �ł͒e����Ă��܂��̂ŁA���ǑS�Ă̗񋓂��K�v�B
    fnGeneric.addAccept(MFnData::kAny);
    fnGeneric.addAccept(MFnData::kNumeric);  // ����͖����Ă����v���������ꉞ�B
    fnGeneric.addAccept(MFnNumericData::k2Short);
    fnGeneric.addAccept(MFnNumericData::k3Short);
    fnGeneric.addAccept(MFnNumericData::k2Long);
    fnGeneric.addAccept(MFnNumericData::k3Long);
    fnGeneric.addAccept(MFnNumericData::k2Float);
    fnGeneric.addAccept(MFnNumericData::k3Float);
    fnGeneric.addAccept(MFnNumericData::k2Double);
    fnGeneric.addAccept(MFnNumericData::k3Double);
    fnGeneric.addAccept(MFnNumericData::k4Double);
    fnGeneric.addAccept(MFnData::kPlugin);
    fnGeneric.addAccept(MFnData::kPluginGeometry);
    fnGeneric.addAccept(MFnData::kString);
    fnGeneric.addAccept(MFnData::kMatrix);
    fnGeneric.addAccept(MFnData::kStringArray);
    fnGeneric.addAccept(MFnData::kDoubleArray);
    fnGeneric.addAccept(MFnData::kIntArray);
    fnGeneric.addAccept(MFnData::kPointArray);
    fnGeneric.addAccept(MFnData::kVectorArray);
    fnGeneric.addAccept(MFnData::kComponentList);
    fnGeneric.addAccept(MFnData::kMesh);
    fnGeneric.addAccept(MFnData::kLattice);
    fnGeneric.addAccept(MFnData::kNurbsCurve);
    fnGeneric.addAccept(MFnData::kNurbsSurface);
    fnGeneric.addAccept(MFnData::kSphere);
    fnGeneric.addAccept(MFnData::kDynArrayAttrs);
    fnGeneric.addAccept(MFnData::kSubdSurface);
    // �ȉ��̓N���b�V������B
    //fnGeneric.addAccept(MFnData::kDynSweptGeometry);
    //fnGeneric.addAccept(MFnData::kNObject);
    //fnGeneric.addAccept(MFnData::kNId);

    fnGeneric.setArray(true);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aInput));

    // output
    aOutput = fnGeneric.create("output", "o");
    // �o�͂̏ꍇ�� kAny �����ł� Numeric �� NumericData �ȊO�̃R�l�N�g�͋��e����Ȃ��B
    // setAttr �͕s�v�Ȃ̂ŁA���͂ƈႢ NumericData �̗񋓂܂ł͕s�v�Ƃ���B
    fnGeneric.addAccept(MFnData::kAny);
    //fnGeneric.addAccept(MFnData::kNumeric);
    //fnGeneric.addAccept(MFnNumericData::k2Short);
    //fnGeneric.addAccept(MFnNumericData::k3Short);
    //fnGeneric.addAccept(MFnNumericData::k2Long);
    //fnGeneric.addAccept(MFnNumericData::k3Long);
    //fnGeneric.addAccept(MFnNumericData::k2Float);
    //fnGeneric.addAccept(MFnNumericData::k3Float);
    //fnGeneric.addAccept(MFnNumericData::k2Double);
    //fnGeneric.addAccept(MFnNumericData::k3Double);
    //fnGeneric.addAccept(MFnNumericData::k4Double);
    fnGeneric.addAccept(MFnData::kPlugin);
    fnGeneric.addAccept(MFnData::kPluginGeometry);
    fnGeneric.addAccept(MFnData::kString);
    fnGeneric.addAccept(MFnData::kMatrix);
    fnGeneric.addAccept(MFnData::kStringArray);
    fnGeneric.addAccept(MFnData::kDoubleArray);
    fnGeneric.addAccept(MFnData::kIntArray);
    fnGeneric.addAccept(MFnData::kPointArray);
    fnGeneric.addAccept(MFnData::kVectorArray);
    fnGeneric.addAccept(MFnData::kComponentList);
    fnGeneric.addAccept(MFnData::kMesh);
    fnGeneric.addAccept(MFnData::kLattice);
    fnGeneric.addAccept(MFnData::kNurbsCurve);
    fnGeneric.addAccept(MFnData::kNurbsSurface);
    fnGeneric.addAccept(MFnData::kSphere);
    fnGeneric.addAccept(MFnData::kDynArrayAttrs);
    fnGeneric.addAccept(MFnData::kSubdSurface);
    // �ȉ��� input �ƈ���ăN���b�V���͂��Ȃ��悤�����s�v���낤�B
    //fnGeneric.addAccept(MFnData::kDynSweptGeometry);
    //fnGeneric.addAccept(MFnData::kNObject);
    //fnGeneric.addAccept(MFnData::kNId);

    fnGeneric.setWritable(false);
    fnGeneric.setStorable(false);
    fnGeneric.setArray(true);
    fnGeneric.setDisconnectBehavior(MFnAttribute::kDelete);
    CHECK_MSTATUS_AND_RETURN_IT(addAttribute(aOutput));

    // Set the attribute dependencies.
    attributeAffects(aCode, aCompiled);
    attributeAffects(aCode, aOutput);
    attributeAffects(aInputsAsApi1Object, aOutput);
    attributeAffects(aInput, aOutput);

    return MS::kSuccess;
}


//------------------------------------------------------------------------------
/// Constructor.
//------------------------------------------------------------------------------
Exprespy::Exprespy() :
    _base_globals(NULL),
    _input(NULL),
    _output(NULL),
    _globals(NULL),
    _codeobj(NULL),
    _mplug2_input(NULL),
    _mplug2_output(NULL),
    _mplug1_input(NULL),
    _mplug1_output(NULL),

    _api2_dict(NULL),
    _type2_MVector(NULL),
    _type2_MPoint(NULL),
    _type2_MEulerRotation(NULL),
    _type2_MMatrix(NULL),
    _type2_MObject(NULL),
    _type2_MQuaternion(NULL),
    _type2_MColor(NULL),
    _type2_MFloatVector(NULL),
    _type2_MFloatPoint(NULL),

    _api1_dict(NULL),
    _type1_MObject(NULL),

    _type_StringIO(NULL),
    _sys_dict(NULL),
    _sys_stdout(NULL),
    _sys_stderr(NULL),

    _count(0)
{
}


//------------------------------------------------------------------------------
/// Destructor.
//------------------------------------------------------------------------------
Exprespy::~Exprespy()
{
    if (_base_globals) {
        PyGILState_STATE gil = PyGILState_Ensure();

        Py_XDECREF(_sys_stdout);
        Py_XDECREF(_sys_stderr);

        Py_XDECREF(_mplug2_input);
        Py_XDECREF(_mplug2_output);
        Py_XDECREF(_mplug1_input);
        Py_XDECREF(_mplug1_output);

        Py_XDECREF(_codeobj);
        Py_XDECREF(_globals);

        Py_DECREF(_output);
        Py_DECREF(_input);
        Py_DECREF(_base_globals);

        PyGILState_Release(gil);
    }
}


//------------------------------------------------------------------------------
/// Computation.
//------------------------------------------------------------------------------
MStatus Exprespy::compute(const MPlug& plug, MDataBlock& block)
{
    if (plug == aCompiled) {
        return _compileCode(block);
    } else if (plug == aOutput) {
        return _executeCode(block);
    }
    return MS::kUnknownParameter;
}


//------------------------------------------------------------------------------
/// Compile a code.
//------------------------------------------------------------------------------
MStatus Exprespy::_compileCode(MDataBlock& block)
{
    MString code = block.inputValue(aCode).asString();

    //MGlobal::displayInfo("COMPILE");

    // Python �̏����J�n�iGIL�擾�j�B
    PyGILState_STATE gil = PyGILState_Ensure();

    // ��x�\�z�ς݂Ȃ�Â��R���p�C���ς݃R�[�h��j���B
    if (_base_globals) {
        Py_XDECREF(_codeobj);
        Py_XDECREF(_globals);

    // ���߂ĂȂ���\�z����B
    } else {
        PyObject* builtins = PyImport_ImportModule("__builtin__");
        if (builtins) {
            // ���[�J���� (globals) �̃x�[�X���\�z����B
            // ����͏����������Ȃ��悤�ɕێ����A�e�R�[�h�����ɂ͂���𕡐����ė��p����B
            _base_globals = PyDict_New();
            _input = PyDict_New();
            _output = PyDict_New();

            if (_base_globals && _input && _output) {
                // �g�ݍ��ݎ������Z�b�g�B
                _setDictStealValue(_base_globals, "__builtins__", builtins);
                _setDictStealValue(_base_globals, "__name__", PyString_FromString("__exprespy__"));

                // �m�[�h�̓��o�͂̂��߂� dict �𐶐��B
                PyDict_SetItemString(_base_globals, "IN", _input);
                PyDict_SetItemString(_base_globals, "OUT", _output);

                // ���W���[�����C���|�[�g�� globals �ɃZ�b�g�B
                PyObject* mod_api2 = _importModule("maya.api.OpenMaya");
                _setDictStealValue(_base_globals, "api", mod_api2);
                _setDictStealValue(_base_globals, "math", _importModule("math"));
                _setDictStealValue(_base_globals, "cmds", _importModule("maya.cmds"));
                _setDictStealValue(_base_globals, "mel", _importModule("maya.mel"));
                PyObject* mod_api1 = _importModule("maya.OpenMaya");
                _setDictStealValue(_base_globals, "api1", mod_api1);
                PyObject* mod_sys = _importModule("sys");
                _setDictStealValue(_base_globals, "sys", mod_sys);

                // Python API ����N���X���擾���Ă����B�u�����Ȃ�Ȃ��O��v�� INCREF �����ɕێ�����B
                // NOTE: �����Ȃ邱�Ƃ��L�蓾��i�Ⴆ�΁A�Q�Ƃ�j�����ăC���|�[�g�������j�ƍl����Ə��X�댯�ł͂���B
                if (mod_api2) {
                    _api2_dict = PyModule_GetDict(mod_api2);
                    _type2_MVector = _getTypeObject(_api2_dict, "MVector");
                    _type2_MPoint = _getTypeObject(_api2_dict, "MPoint");
                    _type2_MEulerRotation = _getTypeObject(_api2_dict, "MEulerRotation");
                    _type2_MMatrix = _getTypeObject(_api2_dict, "MMatrix");
                    _type2_MObject = _getTypeObject(_api2_dict, "MObject");
                    _type2_MQuaternion = _getTypeObject(_api2_dict, "MQuaternion");
                    _type2_MColor = _getTypeObject(_api2_dict, "MColor");
                    _type2_MFloatVector = _getTypeObject(_api2_dict, "MFloatVector");
                    _type2_MFloatPoint = _getTypeObject(_api2_dict, "MFloatPoint");
                }
                if (mod_api1) {
                    _api1_dict = PyModule_GetDict(mod_api1);
                    _type1_MObject = _getTypeObject(_api1_dict, "MObject");
                }

                // �o�̓X�g���[���������邽�߂̏����B
                if (mod_sys) {
                    PyObject* mod_StringIO = _importModule("StringIO");  // cStringIO �ɂ��Ȃ��̂� unicode ��ʂ����߁B
                    if (mod_StringIO) {
                        _type_StringIO = PyDict_GetItemString(PyModule_GetDict(mod_StringIO), "StringIO");
                        if (_type_StringIO) {
                            // stdout �� stdin �� INCREF ���Ă����Ȃ��ƁA�����������ɃN���b�V�������肷��B
                            _sys_dict = PyModule_GetDict(mod_sys);
                            _sys_stdout = PyDict_GetItemString(_sys_dict, "stdout");
                            if (_sys_stdout) Py_INCREF(_sys_stdout);
                            _sys_stderr = PyDict_GetItemString(_sys_dict, "stderr");
                            if (_sys_stderr) Py_INCREF(_sys_stderr);
                        }
                    }
                }

            } else {
                // ���\�z�Ɏ��s�B
                Py_CLEAR(_base_globals);
                Py_CLEAR(_input);
                Py_CLEAR(_output);
            }
        }
    }

    // ���[�J�����̕����ƁA�\�[�X�R�[�h�̃R���p�C���B
    if (_base_globals) {
        _globals = PyDict_Copy(_base_globals);
        if (_globals) {
            _codeobj = reinterpret_cast<PyCodeObject*>(Py_CompileString(code.asChar(), "exprespy_code", Py_file_input));
            if(PyErr_Occurred()){
                _printPythonError();
            }
        }
        _count = 0;
    }

    // Python �̏����I���iGIL����j�B
    PyGILState_Release(gil);

    // �R���p�C���̐��ۂ��Z�b�g�B
    //MGlobal::displayInfo(_codeobj ? "successfull" : "failed");
    block.outputValue(aCompiled).set(_codeobj ? true : false);
    return MS::kSuccess;
}


//------------------------------------------------------------------------------
/// Execute a code.
//------------------------------------------------------------------------------
MStatus Exprespy::_executeCode(MDataBlock& block)
{
    block.inputValue(aCompiled);

    //MGlobal::displayInfo("EXECUTE");

    MArrayDataHandle hArrOutput = block.outputArrayValue(aOutput);

    if (_codeobj) {
        // GIL�擾�O�ɏ㗬�]�����I����B
        const bool inputsAsApi1Object = block.inputValue(aInputsAsApi1Object).asBool();
        MArrayDataHandle hArrInput = block.inputArrayValue(aInput);
        // ���̒i�K�ŏ㗬���]������I����Ă���B

        // Python �̏����J�n�iGIL�擾�j�B
        PyGILState_STATE gil = PyGILState_Ensure();

        // .input[i] ����l�𓾂� IN[i] �ɃZ�b�g����B
        if (hArrInput.elementCount()) {
            do {
                const unsigned idx = hArrInput.elementIndex();

                MDataHandle hInput = hArrInput.inputValue();
                if (hInput.type() == MFnData::kNumeric) {
                    // generic �ł́A�c�O�Ȃ��琔�l�^�̏ڍׂ͔��ʂł��Ȃ��B
                    _setInputScalar(idx, hInput.asGenericDouble());
                } else {
                    MObject data = hInput.data();
                    switch (data.apiType()) {
                    case MFn::kMatrixData:  // matrix --> MMatrix (API2)
                        _setInputMatrix(idx, data);
                        break;
                    case MFn::kStringData:  // string --> unicode
                        _setInputString(idx, hInput.asString());
                        break;
                    case MFn::kData2Short:  // short2 --> list
                        _setInputShort2(idx, data);
                        break;
                    case MFn::kData3Short:  // short3 --> list
                        _setInputShort3(idx, data);
                        break;
                    case MFn::kData2Long:  // long2 --> list
                        _setInputLong2(idx, data);
                        break;
                    case MFn::kData3Long:  // long3 --> list
                        _setInputLong3(idx, data);
                        break;
                    case MFn::kData2Float:  // float2 --> list
                        _setInputFloat2(idx, data);
                        break;
                    case MFn::kData3Float:  // float3 --> list
                        _setInputFloat3(idx, data);
                        break;
                    case MFn::kData2Double:  // double2 --> list
                        _setInputDouble2(idx, data);
                        break;
                    case MFn::kData3Double:  // double3 --> MVector (API2)
                        _setInputVector3(idx, data);
                        break;
                    case MFn::kData4Double:  // double3 --> list
                        _setInputDouble4(idx, data);
                        break;
                    default:                 // other data --> MObject (API2 or 1)
                        //MGlobal::displayInfo(data.apiTypeStr());
                        if (! data.hasFn(MFn::kData)) {
                            // �ڑ��� undo ���Ȃǂ� kInvalid ������BisNull() �ł�����o�������A�ꉞ data �����ʂ��悤�ɂ���B
                            _setDictValue(_input, idx, Py_None);
                        } else if (inputsAsApi1Object) {
                            _setInputMObject1(idx);
                        } else {
                            _setInputMObject2(idx);
                        }
                    }
                }
            } while (hArrInput.next());
        }

        // �J�E���^�[���Z�b�g�B
        _setDictStealValue(_globals, "COUNT", PyInt_FromLong(_count));
        ++_count;

        // sys.stdout �� StringIO �ŒD���B
        // ���̂� print �̏o�͍s���t�]���邱�Ƃ�����̂ŁA�o�b�t�@�ɗ��߂Ĉ��� write ����悤�ɂ���B
        // python �� print �� MGloba::displayInfo �� Maya �ɐ��䂪�Ԃ���Ȃ����� flush ����Ȃ��̂ŁA���̓_�͕ς��Ȃ��B
        // �Ȃ��A���̋t�]���ۂ͋N������N���Ȃ������肵�A�����͂悭������Ȃ��BMaya 2017 win64 �ł����炩�����ƊȒP�ɍČ��o�����B
        PyObject* stream = NULL;
        if (_sys_stdout && _type_StringIO) {
            stream = PyObject_CallObject(_type_StringIO, NULL);
            if (stream) {
                PyDict_SetItemString(_sys_dict, "stdout", stream);
            }
        }

        // �R���p�C���ς݃R�[�h�I�u�W�F�N�g�����s�B
        PyEval_EvalCode(_codeobj, _globals, NULL);
        if(PyErr_Occurred()){
            _printPythonError();
        }

        // StringIO �̒��g��{���� sys.stdout �ɏ����o���Asys.stdout �����ɖ߂��B
        // MGlobal::displayInfo ��p���Ȃ��̂́A�R�����g�����������Ƀ_�C���N�g�� print �����邽�߁B
        if (stream) {
            PyObject* str = PyObject_CallMethod(stream, "getvalue", NULL);
            if (PyString_GET_SIZE(str)) {
                PyObject_CallMethod(_sys_stdout, "write", "(O)", str);
            }
            Py_DECREF(str);
            PyDict_SetItemString(_sys_dict, "stdout", _sys_stdout);
            PyObject_CallMethod(stream, "close", NULL);
            Py_DECREF(stream);
        }

        // .output[i] �� OUT[i] �̒l���Z�b�g����B���݂��Ȃ����̂∵���Ȃ����̏ꍇ�͖�������B
        if (hArrOutput.elementCount()) {
            MMatrix mat;
            do {
                const unsigned idx = hArrOutput.elementIndex();

                PyObject* valo = _getDictValue(_output, idx);  // �ێ����Ȃ��̂� INCREF ���Ȃ��B
                if (! valo) continue;

                // float --> double
                if (PyFloat_Check(valo)) {
                    hArrOutput.outputValue().setGenericDouble(PyFloat_AS_DOUBLE(valo), true);

                // int --> int
                } else if (PyInt_Check(valo)) {
                    hArrOutput.outputValue().setGenericInt(PyInt_AS_LONG(valo), true);

                // long int --> int
                } else if (PyLong_Check(valo)) {
                    hArrOutput.outputValue().setGenericInt(PyLong_AsLong(valo), true);

                // bool --> bool
                } else if (PyBool_Check(valo)) {
                    hArrOutput.outputValue().setGenericBool(valo == Py_True, true);

                // str --> string
                } else if (PyString_Check(valo)) {
                    hArrOutput.outputValue().set(MString(PyString_AsString(valo)));

                // unicode --> string
                } else if (PyUnicode_Check(valo)) {
                    //PyObject* es = PyUnicode_AsUTF8String(valo);
                    PyObject* es = PyUnicode_AsEncodedString(valo, Py_FileSystemDefaultEncoding, NULL);
                    if (es) {
                        hArrOutput.outputValue().set(MString(PyString_AsString(es)));
                        Py_DECREF(es);
                    } else {
                        hArrOutput.outputValue().set(MString());
                    }

                // MMatrix (API2) --> matrix
                } else if (_type2_MMatrix && PyObject_IsInstance(valo, _type2_MMatrix)) {
                    _getMatrixValueFromPyObject(mat, valo);
                    hArrOutput.outputValue().set(MFnMatrixData().create(mat));

                // MVector, MPoint, MEulerRotation (API2) --> double3
                } else if ((_type2_MVector && PyObject_IsInstance(valo, _type2_MVector))
                 || (_type2_MPoint && PyObject_IsInstance(valo, _type2_MPoint))
                 || (_type2_MEulerRotation && PyObject_IsInstance(valo, _type2_MEulerRotation))) {
                    hArrOutput.outputValue().set(_getDouble3Value(valo));

                // MQuaternion (API2) --> double4
                } else if (_type2_MQuaternion && PyObject_IsInstance(valo, _type2_MQuaternion)) {
                    hArrOutput.outputValue().set(_getDouble4Value(valo));

                // MFloatVector, MFloatPoint, MColor (API2) --> float3
                } else if ((_type2_MFloatVector && PyObject_IsInstance(valo, _type2_MFloatVector))
                 || (_type2_MFloatPoint && PyObject_IsInstance(valo, _type2_MFloatPoint))
                 || (_type2_MColor && PyObject_IsInstance(valo, _type2_MColor))) {
                    hArrOutput.outputValue().set(_getFloat3Value(valo));

                // sequence --> double2, double3, double4, long2, long3, or null
                } else if (PySequence_Check(valo)) {
                    hArrOutput.outputValue().set(_getTypedSequence(valo));

                // MObject (API2 or 1) --> data
                } else if (_type2_MObject && PyObject_IsInstance(valo, _type2_MObject)) {
                    _setOutputMObject2(idx, valo);
                } else if (_type1_MObject && PyObject_IsInstance(valo, _type1_MObject)) {
                    _setOutputMObject1(idx, valo);
                }
            } while (hArrOutput.next());
        }

        // ����̂��߂� IN �� OUT ���N���A���Ă����B
        PyDict_Clear(_input);
        PyDict_Clear(_output);

        // Python �̏����I���iGIL����j�B
        PyGILState_Release(gil);
    }

    return hArrOutput.setAllClean();
}


//------------------------------------------------------------------------------
/// Python �̃G���[�� Maya �̃G���[���b�Z�[�W�Ƃ��ďo�͂���B
/// ���b�Z�[�W��Ԃ����鋤�ɁA���̂��s�̏������t�]����ꍇ������̂��������B
//------------------------------------------------------------------------------
void Exprespy::_printPythonError()
{
    // sys.stderr �� StringIO �ŒD���B
    PyObject* stream = NULL;
    if (_sys_stderr && _type_StringIO) {
        stream = PyObject_CallObject(_type_StringIO, NULL);
        if (stream) {
            PyDict_SetItemString(_sys_dict, "stderr", stream);
        }
    }

    // ���݂̃G���[���� stderr �ɏ����o���B
    PyErr_Print();

    // StringIO �̒��g��{���� sys.stderr �ɏ����o���Asys.stderr �����ɖ߂��B
    if (stream) {
        PyObject* str = PyObject_CallMethod(stream, "getvalue", NULL);
        if (PyString_GET_SIZE(str)) {
            MGlobal::displayError(PyString_AsString(str));
        }
        Py_DECREF(str);
        PyDict_SetItemString(_sys_dict, "stderr", _sys_stderr);
        PyObject_CallMethod(stream, "close", NULL);
        Py_DECREF(stream);
    }
}


//------------------------------------------------------------------------------
/// _input dict �ɃX�J���[���l���Z�b�g����B
//------------------------------------------------------------------------------
void Exprespy::_setInputScalar(int key, const double& val)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        // ���͂���^���ʏo���Ȃ����߁A���߂Đ����Ǝ����𔻕ʂ���B
        int i = static_cast<int>(val);
        if (static_cast<double>(i) == val) {
            _setIODictStealValue(_input, keyo, PyInt_FromLong(i));
        } else {
            _setIODictStealValue(_input, keyo, PyFloat_FromDouble(val));
        }
    }
}


//------------------------------------------------------------------------------
/// _input dict �ɕ�������Z�b�g����B
//------------------------------------------------------------------------------
void Exprespy::_setInputString(int key, const MString& str)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        //_setIODictStealValue(_input, keyo, PyString_FromString(str.asChar()));
        _setIODictStealValue(_input, keyo, PyUnicode_FromString(str.asUTF8()));
    }
}


//------------------------------------------------------------------------------
/// _input dict �ɗv�f�� 2�`4 �̒l�� list �ɂ��ăZ�b�g����B
//------------------------------------------------------------------------------
void Exprespy::_setInputShort2(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        short x, y;
        MFnNumericData(data).getData(x, y);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[hh]", x, y));
    }
}

void Exprespy::_setInputShort3(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        short x, y, z;
        MFnNumericData(data).getData(x, y, z);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[hhh]", x, y, z));
    }
}

void Exprespy::_setInputLong2(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        int x, y;
        MFnNumericData(data).getData(x, y);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[ii]", x, y));
    }
}

void Exprespy::_setInputLong3(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        int x, y, z;
        MFnNumericData(data).getData(x, y, z);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[iii]", x, y, z));
    }
}

void Exprespy::_setInputFloat2(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        float x, y;
        MFnNumericData(data).getData(x, y);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[ff]", x, y));
    }
}

void Exprespy::_setInputFloat3(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        float x, y, z;
        MFnNumericData(data).getData(x, y, z);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[fff]", x, y, z));
    }
}

void Exprespy::_setInputDouble2(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        double x, y;
        MFnNumericData(data).getData(x, y);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[dd]", x, y));
    }
}

void Exprespy::_setInputDouble3(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        double x, y, z;
        MFnNumericData(data).getData(x, y, z);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[ddd]", x, y, z));
    }
}

void Exprespy::_setInputDouble4(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        double x, y, z, w;
        MFnNumericData(data).getData(x, y, z, w);
        _setIODictStealValue(_input, keyo, Py_BuildValue("[dddd]", x, y, z, w));
    }
}


//------------------------------------------------------------------------------
/// _input dict �� double3 �� MVector �� list �ɂ��ăZ�b�g����B
//------------------------------------------------------------------------------
void Exprespy::_setInputVector3(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        double x, y, z;
        MFnNumericData(data).getData(x, y, z);
        if (_type2_MVector) {
            _setIODictStealValue(_input, keyo, PyObject_CallFunction(_type2_MVector, "(ddd)", x, y, z));
        } else {
            _setIODictStealValue(_input, keyo, Py_BuildValue("[ddd]", x, y, z));
        }
    }
}


//------------------------------------------------------------------------------
/// _input dict �� matrix �� MMatrix �� list �ɂ��ăZ�b�g����B
//------------------------------------------------------------------------------
void Exprespy::_setInputMatrix(int key, MObject& data)
{
    PyObject* keyo = PyInt_FromLong(key);
    if (keyo) {
        MMatrix m = MFnMatrixData(data).matrix();
        if (_type2_MMatrix) {
            _setIODictStealValue(_input, keyo, PyObject_CallFunction(_type2_MMatrix, "(N)", Py_BuildValue(
                "(dddddddddddddddd)",
                m.matrix[0][0], m.matrix[0][1], m.matrix[0][2], m.matrix[0][3],
                m.matrix[1][0], m.matrix[1][1], m.matrix[1][2], m.matrix[1][3],
                m.matrix[2][0], m.matrix[2][1], m.matrix[2][2], m.matrix[2][3],
                m.matrix[3][0], m.matrix[3][1], m.matrix[3][2], m.matrix[3][3]
            )));
        } else {
            _setIODictStealValue(_input, keyo, Py_BuildValue(
                "[dddddddddddddddd]",
                m.matrix[0][0], m.matrix[0][1], m.matrix[0][2], m.matrix[0][3],
                m.matrix[1][0], m.matrix[1][1], m.matrix[1][2], m.matrix[1][3],
                m.matrix[2][0], m.matrix[2][1], m.matrix[2][2], m.matrix[2][3],
                m.matrix[3][0], m.matrix[3][1], m.matrix[3][2], m.matrix[3][3]
            ));
        }
    }
}


//------------------------------------------------------------------------------
/// _input dict �� Python API 2 MObject �f�[�^���Z�b�g����B
/// MObjet �� C++ ���� Python �ɕϊ������i�������̂ŁA�s�{�ӂȂ��� MPlug �𗘗p�B
//------------------------------------------------------------------------------
void Exprespy::_setInputMObject2(int key)
{
    _preparePyPlug2();
    if (! _mplug2_input) return;

    PyObject* keyo = PyInt_FromLong(key);
    if (! keyo) return;

    PyObject* mplug = PyObject_CallMethod(_mplug2_input, "elementByLogicalIndex", "(O)", keyo);
    if (mplug) {
        _setIODictStealValue(_input, keyo, PyObject_CallMethod(mplug, "asMObject", NULL));
        Py_DECREF(mplug);
    } else {
        Py_DECREF(keyo);
    }
}


//------------------------------------------------------------------------------
/// _output dict ���瓾�� Python API 2 MObject �f�[�^���o�̓v���O�ɃZ�b�g����B
/// MObjet �� Python ���� C++ �ɕϊ������i�������̂ŁA�s�{�ӂȂ��� MPlug �𗘗p�B
//------------------------------------------------------------------------------
void Exprespy::_setOutputMObject2(int key, PyObject* valo)
{
    _preparePyPlug2();
    if (! _mplug2_output) return;

    PyObject* mplug = PyObject_CallMethod(_mplug2_output, "elementByLogicalIndex", "(i)", key);
    if (mplug) {
        PyObject_CallMethod(mplug, "setMObject", "(O)", valo);
        Py_DECREF(mplug);
    }
}


//------------------------------------------------------------------------------
/// MObject �f�[�^���o�͂̂��߂� Python API 2 MPlug ���擾���Ă����B
//------------------------------------------------------------------------------
void Exprespy::_preparePyPlug2()
{
    if (_mplug2_input || ! _api2_dict) return;

    PyObject* pysel = PyObject_CallObject(PyDict_GetItemString(_api2_dict, "MSelectionList"), NULL);
    if (! pysel) return;

    MString nodename = MFnDependencyNode(thisMObject()).name();
    PyObject_CallMethod(pysel, "add", "(s)", nodename.asChar());
    PyObject* pynode = PyObject_CallMethod(pysel, "getDependNode", "(i)", 0);
    Py_DECREF(pysel);
    if (! pynode) return;

    PyObject* pyfn = PyObject_CallFunction(PyDict_GetItemString(_api2_dict, "MFnDependencyNode"), "(O)", pynode);
    Py_DECREF(pynode);
    if (! pyfn) return;

    PyObject* findPlug = PyObject_GetAttrString(pyfn, "findPlug");  // DECREF �s�v�B
    _mplug2_input = PyObject_CallFunction(findPlug, "(sO)", "i", Py_False);
    _mplug2_output = PyObject_CallFunction(findPlug, "(sO)", "o", Py_False);
    Py_DECREF(pyfn);
}


//------------------------------------------------------------------------------
/// _input dict �� Python API 1 MObject �f�[�^���Z�b�g����B
/// MObjet �� C++ ���� Python �ɕϊ������i�������̂ŁA�s�{�ӂȂ��� MPlug �𗘗p�B
//------------------------------------------------------------------------------
void Exprespy::_setInputMObject1(int key)
{
    _preparePyPlug1();
    if (! _mplug1_input) return;

    PyObject* keyo = PyInt_FromLong(key);
    if (! keyo) return;

    PyObject* mplug = PyObject_CallMethod(_mplug1_input, "elementByLogicalIndex", "(O)", keyo);
    if (! mplug) { Py_DECREF(keyo); return; }

    PyObject* valo = PyObject_CallMethod(mplug, "asMObject", NULL);
    Py_DECREF(mplug);

    _setIODictStealValue(_input, keyo, valo);
}


//------------------------------------------------------------------------------
/// _output dict ���瓾�� Python API 1 MObject �f�[�^���o�̓v���O�ɃZ�b�g����B
/// MObjet �� Python ���� C++ �ɕϊ������i�������̂ŁA�s�{�ӂȂ��� MPlug �𗘗p�B
//------------------------------------------------------------------------------
void Exprespy::_setOutputMObject1(int key, PyObject* valo)
{
    _preparePyPlug1();
    if (! _mplug1_output) return;

    PyObject* mplug = PyObject_CallMethod(_mplug1_output, "elementByLogicalIndex", "(i)", key);
    if (mplug) {
        PyObject_CallMethod(mplug, "setMObject", "(O)", valo);
        Py_DECREF(mplug);
    }
}


//------------------------------------------------------------------------------
/// MObject �f�[�^���o�͂̂��߂� Python API 1 MPlug ���擾���Ă����B
//------------------------------------------------------------------------------
void Exprespy::_preparePyPlug1()
{
    if (_mplug1_input || ! _api1_dict) return;

    PyObject* pysel = PyObject_CallObject(PyDict_GetItemString(_api1_dict, "MSelectionList"), NULL);
    if (! pysel) return;

    MString nodename = MFnDependencyNode(thisMObject()).name();
    PyObject_CallMethod(pysel, "add", "(s)", nodename.asChar());
    PyObject* pynode = PyObject_CallObject(_type1_MObject, NULL);
    if (! pynode) return;
    PyObject_CallMethod(pysel, "getDependNode", "(iO)", 0, pynode);
    Py_DECREF(pysel);

    if (PyObject_CallMethod(pynode, "isNull", NULL) != Py_True) {
        PyObject* pyfn = PyObject_CallFunction(PyDict_GetItemString(_api1_dict, "MFnDependencyNode"), "(O)", pynode);

        PyObject* findPlug = PyObject_GetAttrString(pyfn, "findPlug");  // DECREF �s�v�B
        if (findPlug) {
            _mplug1_input = PyObject_CallFunction(findPlug, "(sO)", "i", Py_False);
            _mplug1_output = PyObject_CallFunction(findPlug, "(sO)", "o", Py_False);
            if (PyObject_CallMethod(_mplug1_input, "isNull", NULL) == Py_True) Py_CLEAR(_mplug1_input);
            if (PyObject_CallMethod(_mplug1_output, "isNull", NULL) == Py_True) Py_CLEAR(_mplug1_output);
        }

        Py_DECREF(pyfn);
    }
    Py_DECREF(pynode);
}


//=============================================================================
//  ENTRY POINT FUNCTIONS
//=============================================================================
MStatus initializePlugin(MObject obj)
{ 
    static const char* VERSION = "2.0.0.20161029";
    static const char* VENDER  = "Ryusuke Sasaki";

    MFnPlugin plugin(obj, VENDER, VERSION, "Any");

    MStatus stat = plugin.registerNode(Exprespy::name, Exprespy::id, Exprespy::creator, Exprespy::initialize);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus stat = MFnPlugin(obj).deregisterNode(Exprespy::id);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    return MS::kSuccess;
}

