/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 *****************************************************************************/
#include "jpype.h"
#include "jp_primitive_accessor.h"
#include "jp_booleantype.h"
#include "pyjp.h"

JPBooleanType::JPBooleanType()
: JPPrimitiveType("boolean")
{
}

JPBooleanType::~JPBooleanType()
{
}

JPPyObject JPBooleanType::convertToPythonObject(JPJavaFrame& frame, jvalue val)
{
	return JPPyObject(JPPyRef::_call, PyBool_FromLong(val.z));
}

JPValue JPBooleanType::getValueFromObject(const JPValue& obj)
{
	JPContext *context = obj.getClass()->getContext();
	JPJavaFrame frame(context);
	jvalue v;
	field(v) = frame.CallBooleanMethodA(obj.getValue().l, context->m_BooleanValueID, 0) ? true : false;
	return JPValue(this, v);
}

class JPConversionAsBoolean : public JPConversion
{
public:

	virtual jvalue convert(JPJavaFrame* frame, JPClass* cls, PyObject* pyobj) override
	{
		jvalue res;
		res.z = JPPyLong::asLong(pyobj) != 0;
		return res;
	}
} asBooleanConversion;

JPMatch::Type JPBooleanType::getJavaConversion(JPJavaFrame *frame, JPMatch &match, PyObject *pyobj)
{
	JP_TRACE_IN("JPBooleanType::getJavaConversion", this);
	JPContext *context = NULL;
	if (frame != NULL)
		context = frame->getContext();

	if (JPPyObject::isNone(pyobj))
		return match.type = JPMatch::_none;

			if (PyBool_Check(pyobj))
	{
		match.conversion = &asBooleanConversion;
		return match.type = JPMatch::_exact;
			}

	JPValue *value = PyJPValue_getJavaSlot(pyobj);
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls == NULL)
			return match.type = JPMatch::_none;

		if (cls == this)
		{
			match.conversion = javaValueConversion;
			return match.type = JPMatch::_exact;
		}

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (context != NULL && cls == context->_java_lang_Boolean)
		{
			match.conversion = unboxConversion;
			return match.type = JPMatch::_implicit;
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return match.type = JPMatch::_none;
	}

	if (JPPyLong::check(pyobj))
	{
		match.conversion = &asBooleanConversion;
		return match.type = JPMatch::_implicit;
	}

	if (JPPyLong::checkConvertable(pyobj))
	{
		match.conversion = &asBooleanConversion;
		match.type = JPPyLong::checkIndexable(pyobj) ? JPMatch::_implicit : JPMatch::_explicit;
		return match.type;
	}

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

jarray JPBooleanType::newArrayInstance(JPJavaFrame& frame, jsize sz)
{
	return frame.NewBooleanArray(sz);
}

JPPyObject JPBooleanType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticBooleanField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPBooleanType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetBooleanField(c, fid);
	return convertToPythonObject(frame, v);
}

JPPyObject JPBooleanType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticBooleanMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v);
}

JPPyObject JPBooleanType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == NULL)
			field(v) = frame.CallBooleanMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualBooleanMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v);
}

void JPBooleanType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetStaticBooleanField(c, fid, val);
}

void JPBooleanType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetBooleanField(c, fid, val);
}

void JPBooleanType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPBooleanType::setArrayRange");
	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetBooleanArrayElements, &JPJavaFrame::ReleaseBooleanArrayElements);

	type_t* val = accessor.get();
	// First check if assigning sequence supports buffer API
	if (PyObject_CheckBuffer(sequence))
	{
		JPPyBuffer buffer(sequence, PyBUF_FULL_RO);
		if (buffer.valid())
		{
			Py_buffer& view = buffer.getView();
			if (view.ndim != 1)
				JP_RAISE(PyExc_TypeError, "buffer dims incorrect");
			Py_ssize_t vshape = view.shape[0];
			Py_ssize_t vstep = view.strides[0];
			if (vshape != length)
				JP_RAISE(PyExc_ValueError, "mismatched size");

			char* memory = (char*) view.buf;
			if (view.suboffsets && view.suboffsets[0] >= 0)
				memory = *((char**) memory) + view.suboffsets[0];
			jsize index = start;
			jconverter conv = getConverter(view.format, (int) view.itemsize, "z");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.z;
				memory += vstep;
		}
			accessor.commit();
			return;
		} else
		{
			PyErr_Clear();
		}
	}

	// Use sequence API
	JPPySequence seq(JPPyRef::_use, sequence);
	jsize index = start;
	for (Py_ssize_t i = 0; i < length; ++i, index += step)
	{
		val[index] = PyObject_IsTrue(seq[i].get());
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPBooleanType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	array_t array = (array_t) a;
	type_t val;
	frame.GetBooleanArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v);
}

void JPBooleanType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match;
	if (getJavaConversion(&frame, match, obj) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java boolean");
	type_t val = field(match.conversion->convert(&frame, this, obj));
	frame.SetBooleanArrayRegion((array_t) a, ndx, 1, &val);
}

string JPBooleanType::asString(jvalue v)
{
	return v.z ? "true" : "false";
}

void JPBooleanType::getView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	view.m_Memory = (void*) frame.GetBooleanArrayElements(
			(jbooleanArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "?";
	view.m_Buffer.itemsize = sizeof (jboolean);
}

void JPBooleanType::releaseView(JPArrayView& view)
{
	JPJavaFrame frame(view.getContext());
	frame.ReleaseBooleanArrayElements((jbooleanArray) view.m_Array->getJava(),
			(jboolean*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
}

const char* JPBooleanType::getBufferFormat()
{
	return "?";
}

ssize_t JPBooleanType::getItemSize()
{
	return sizeof (jboolean);
}

void JPBooleanType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jboolean* b = (jboolean*) ((char*) memory + offset);
	frame.GetBooleanArrayRegion((jbooleanArray) a, start, len, b);
}
