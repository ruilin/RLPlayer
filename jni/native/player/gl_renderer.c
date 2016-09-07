/*
 * gl_renderer.c
 *
 *  Created on: 2016年7月7日
 *      Author: Ruilin
 *
 *   http://blog.csdn.net/cjj198561/article/details/34136187
 *   http://www.xuebuyuan.com/510409.html
 */

#include  "gl_renderer.h"

const char INDICES_YUV[] = { 0, 3, 2, 0, 2, 1 };

const char VERTEX_SHADER_YUV[] = {
		"attribute vec4 aPosition;\n"
		"attribute vec2 aTextureCoord;\n"
		"varying vec2 vTextureCoord;\n"
		"void main() {\n"
		"  gl_Position = aPosition;\n"
		"  vTextureCoord = aTextureCoord;\n"
		"}\n"
};

// The fragment shader.
// Do YUV to RGB565 conversion.
const char FRAGMENT_SHADER_YUV[] = {
		"precision mediump float;\n"
		"uniform sampler2D Ytex;\n"
		"uniform sampler2D Utex,Vtex;\n"
		"varying vec2 vTextureCoord;\n"
		"void main(void) {\n"
		"  float nx,ny,r,g,b,y,u,v;\n"
		"  mediump vec4 txl,ux,vx;"
		"  nx=vTextureCoord[0];\n"
		"  ny=vTextureCoord[1];\n"
		"  y=texture2D(Ytex,vec2(nx,ny)).r;\n"
		"  u=texture2D(Utex,vec2(nx,ny)).r;\n"
		"  v=texture2D(Vtex,vec2(nx,ny)).r;\n"

		//"  y = v;\n"+
		"  y=1.1643*(y-0.0625);\n"
		"  u=u-0.5;\n"
		"  v=v-0.5;\n"

		"  r=y+1.5958*v;\n"
		"  g=y-0.39173*u-0.81290*v;\n"
		"  b=y+2.017*u;\n"
		"  gl_FragColor=vec4(r,g,b,1.0);\n"
		"}\n"
};

const GLfloat _vertices[20] = {
    // X, Y, Z, U, V
//	-1,-1, 0, 1, 0, // Bottom Left
//	1, -1, 0, 0, 0, //Bottom Right
//	1, 1, 0, 0, 1, //Top Right
//	-1, 1, 0, 1, 1, //Top Left

	1, 1, 0, 1, 0, // Top Right
    -1, 1, 0, 0, 0, //Top Left
    -1, -1, 0, 0, 1, //Bottom Left
    1, -1, 0, 1, 1, //Bottom Right
};

GLuint mYuvProgram;
GLuint _textureIds[3];
GLuint _textureWidth;
GLuint _textureHeight;

/*--------------------------------------------------------------------------------------*/
/* 硬解码渲染 shader */
const char VERTEX_SHADER_H264[] = {
		"attribute vec4 vPosition;"
		"attribute vec2 inputTextureCoordinate;"
		"varying vec2 textureCoordinate;"
		"void main()"
		"{"
		"	gl_Position = vPosition;"
		"	textureCoordinate = inputTextureCoordinate;"
		"}"
};

const char FRAGMENT_SHADER_H264[] = {
		"#extension GL_OES_EGL_image_external : require\n"
		"precision mediump float;"
		"varying vec2 textureCoordinate;\n"
		"uniform samplerExternalOES s_texture;\n"
		"void main() {"
		"  gl_FragColor = texture2D( s_texture, textureCoordinate );\n"
		"}"
};

const short INDICES_H264[] = { 0, 1, 2, 0, 2, 3 }; // order to draw vertices
// number of coordinates per vertex in this array
#define COORDS_PER_VERTEX		2
const int vertexStride = COORDS_PER_VERTEX * 4; // 4 bytes per

const float squareCoords[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f,
	-1.0f, 1.0f,
};

const float textureVertices[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
};

GLuint mH264Program;



GL_RENDER_TYPE mRenderType;
jobject jview;
void *hw_deocder;
unsigned char *g_buffer;
unsigned int g_bufferSize;
unsigned short g_width;
unsigned short g_height;
BOOL hasSetYuvProgram;
BOOL hasSetH264Program;

BOOL isCreated;
BOOL isHwRending;

pthread_mutex_t mLock;

void gl_init(JNIEnv *env, GL_RENDER_TYPE defMode, void *deocder) {
	pthread_mutex_init(&mLock, NULL);
	hw_deocder = deocder;
	jview = NULL;
	g_buffer = NULL;
	g_width = 0;
	g_height = 0;
	mRenderType = defMode;
	_textureWidth = -1;
	_textureHeight = -1;
	hasSetYuvProgram = B_FALSE;
	hasSetH264Program = B_FALSE;
	isCreated = B_FALSE;
	isHwRending = B_FALSE;
	return;
}

void gl_uninit(JNIEnv *env) {
	pthread_mutex_lock(&mLock);
	if (jview != NULL) {
		(*env)->DeleteGlobalRef(env, jview);
		jview = NULL;
	}
	pthread_mutex_unlock(&mLock);
	glDeleteTextures(3, _textureIds);
	if (g_buffer != NULL) {
		free(g_buffer);
		g_buffer = NULL;
	}
	return;
}

void gl_render_frame(JNIEnv *env, unsigned char *data, unsigned int len, unsigned short width, unsigned short height) {
	pthread_mutex_lock(&mLock);
	if (jview == NULL) {
		LOGE("gl_render_frame() jview NULL");
		goto RETURN;
	}
	isHwRending = B_FALSE;
	if (g_buffer == NULL || g_width != width || g_height != height) {
		if (g_buffer != NULL) {
			free(g_buffer);
			g_buffer = NULL;
		}
		g_buffer = malloc(sizeof(unsigned char) * len);
		if (g_buffer == NULL) {
			goto RETURN;
		}
	}
	/* 经过JNI调用，可能处于不同线程，data不能直接使用，需要把数组拷贝出来 */
	g_buffer = memcpy(g_buffer, data, len);
	g_bufferSize = len;
	g_width = width;
	g_height = height;
	jclass clazz = (*env)->GetObjectClass(env, jview);
	jmethodID method = (*env)->GetMethodID(env, clazz, "requestRender", "()V");
	(*env)->CallVoidMethod(env, jview, method);
	(*env)->DeleteLocalRef(env, clazz);
	RETURN:
	pthread_mutex_unlock(&mLock);
	return;
}

/**
 * mode:
 * 0 - hardware
 * 1 - software
 */
void gl_set_type(JNIEnv *env, GL_RENDER_TYPE _type) {
	mRenderType = _type;
	switch (mRenderType) {
	case GL_RENDER_HW: {
		if (jview != NULL) {
			jclass clazz = (*env)->GetObjectClass(env, jview);
			jmethodID method = (*env)->GetMethodID(env, clazz, "openHwDecoder", "()V");
			(*env)->CallVoidMethod(env, jview, method);
		}
	} break;
	case GL_RENDER_SW: {
		hw_decoder_close(hw_deocder, env);
	} break;
	default:
		break;
	}

	hasSetYuvProgram = B_FALSE;
	hasSetH264Program = B_FALSE;
	return;
}

BOOL gl_isHwRending() {
	return isHwRending;
}

static GLuint loadShader(GLenum shaderType, const char* pSource) {
	GLuint shader = glCreateShader(shaderType);
	if (shader) {
		glShaderSource(shader, 1, &pSource, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char* buf = (char *)malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					free(buf);
				}
				glDeleteShader(shader);
				shader = 0;
			}
		}
	}
	return shader;
}

static void printGLString(const char *name, GLenum s) {
	const char *v = (const char *) glGetString(s);
	LOGI("GL %s = %s\n", name, v);
}

void checkGlError(const char* op) {
#ifdef ANDROID_LOG
	GLint error;
	for (error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
#else
	return;
#endif
}

static GLuint createProgram(const char* pVertexSource,
												const char* pFragmentSource) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
	if (!vertexShader) {
		return 0;
	}
	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
	if (!pixelShader) {
		return 0;
	}

	GLuint program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char* buf = (char*) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					free(buf);
				}
			}
			glDeleteProgram(program);
			program = 0;
		}
	}
	return program;
}

static void InitializeTexture(int name, int id, int width, int height) {
	glActiveTexture(name);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	//GL_NEAREST
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
			GL_UNSIGNED_BYTE, NULL);
}

static void setupTextures(int32_t width, int32_t height) {
	glDeleteTextures(3, _textureIds);
	glGenTextures(3, _textureIds); //Generate  the Y, U and V texture
	InitializeTexture(GL_TEXTURE0, _textureIds[0], width, height);
	InitializeTexture(GL_TEXTURE1, _textureIds[1], width >> 1, height >> 1);
	InitializeTexture(GL_TEXTURE2, _textureIds[2], width >> 1, height >> 1);

	checkGlError("SetupTextures");

	_textureWidth = width;
	_textureHeight = height;
}

#define SET_TEX_PARAM() 	{																									\
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);					\
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);					\
	    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);		\
	    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);		\
}
/*
 * glTexSubImage2D(): 在一个已经存在的纹理A中，嵌入另外一幅纹理B贴图
 */
static void updateTextures(char *data, int32_t width, int32_t height) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureIds[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    data);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textureIds[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width >> 1, height >> 1, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, (char *)data + width * height);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textureIds[2]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width >> 1, height >> 1, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, (char *)data + width * height * 5 / 4);

    checkGlError("UpdateTextures");

//    glBindTexture(GL_TEXTURE_2D, _textureIds[0]);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
//    SET_TEX_PARAM()
//
//    glBindTexture(GL_TEXTURE_2D, _textureIds[1]);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char *)data + width * height);
//    SET_TEX_PARAM()
//
//    glBindTexture(GL_TEXTURE_2D, _textureIds[2]);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (char *)data + width * height * 5 / 4);
//    SET_TEX_PARAM()
}

static void useYUVProgram(GLuint _program) {
	int maxTextureImageUnits[2];
	int maxTextureSize[2];
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, maxTextureImageUnits);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);


	int positionHandle = glGetAttribLocation(_program, "aPosition");
	checkGlError("glGetAttribLocation aPosition");
	if (positionHandle == -1) {
		return;
	}

	int textureHandle = glGetAttribLocation(_program, "aTextureCoord");
	checkGlError("glGetAttribLocation aTextureCoord");
	if (textureHandle == -1) {
		return;
	}

	// set the vertices array in the shader
	// _vertices contains 4 vertices with 5 coordinates.
	// 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(positionHandle, 3, GL_FLOAT, B_FALSE,
			5 * sizeof(GLfloat), _vertices);
	checkGlError("glVertexAttribPointer aPosition");

	glEnableVertexAttribArray(positionHandle);
	checkGlError("glEnableVertexAttribArray positionHandle");

	// set the texture coordinate array in the shader
	// _vertices contains 4 vertices with 5 coordinates.
	// 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(textureHandle, 2, GL_FLOAT, B_FALSE,
			5 * sizeof(GLfloat), &_vertices[3]);
	checkGlError("glVertexAttribPointer maTextureHandle");
	glEnableVertexAttribArray(textureHandle);
	checkGlError("glEnableVertexAttribArray textureHandle");

	glUseProgram(_program);
	int i = glGetUniformLocation(_program, "Ytex");
	checkGlError("glGetUniformLocation");
	glUniform1i(i, 0); /* Bind Ytex to texture unit 0 */
	checkGlError("glUniform1i Ytex");

	i = glGetUniformLocation(_program, "Utex");
	checkGlError("glGetUniformLocation Utex");
	glUniform1i(i, 1); /* Bind Utex to texture unit 1 */
	checkGlError("glUniform1i Utex");

	i = glGetUniformLocation(_program, "Vtex");
	checkGlError("glGetUniformLocation");
	glUniform1i(i, 2); /* Bind Vtex to texture unit 2 */
	checkGlError("glUniform1i");

	return;
}

static void useH264Program(GLuint _program) {
	glUseProgram(_program);

	int positionHandle = glGetAttribLocation(_program, "vPosition");
	checkGlError("glGetAttribLocation aPosition");
	if (positionHandle == -1) {
		return;
	}
	glEnableVertexAttribArray(positionHandle);
	// set the vertices array in the shader
	// _vertices contains 4 vertices with 5 coordinates.
	// 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(positionHandle, 3, GL_FLOAT, B_FALSE,
			vertexStride, squareCoords);
	checkGlError("glVertexAttribPointer aPosition");

	int textureHandle = glGetAttribLocation(_program, "inputTextureCoordinate");
	checkGlError("glGetAttribLocation inputTextureCoordinate");
	if (textureHandle == -1) {
		return;
	}
	glEnableVertexAttribArray(textureHandle);
	checkGlError("glEnableVertexAttribArray textureHandle");
	// set the texture coordinate array in the shader
	// _vertices contains 4 vertices with 5 coordinates.
	// 3 for (xyz) for the vertices and 2 for the texture
	glVertexAttribPointer(textureHandle, 2, GL_FLOAT, B_FALSE,
			vertexStride, &textureVertices);
	checkGlError("glVertexAttribPointer maTextureHandle");

	return;
}

static int32_t onSetup(int32_t width, int32_t height) {
	printGLString("Version", GL_VERSION);
	printGLString("Vendor", GL_VENDOR);
	printGLString("Renderer", GL_RENDERER);
	printGLString("Extensions", GL_EXTENSIONS);

	glViewport(0, 0, width, height);
	checkGlError("glViewport");

	mYuvProgram = createProgram(VERTEX_SHADER_YUV, FRAGMENT_SHADER_YUV);
	if (!mYuvProgram) {
		return -1;
	}

	mH264Program = createProgram(VERTEX_SHADER_H264, FRAGMENT_SHADER_H264);
	if (!mH264Program) {
		return -1;
	}

//	useYUVProgram(mYuvProgram);
	return 0;
}

static int32_t onRenderYUV(char *data, int32_t width, int32_t height) {
	if (hasSetYuvProgram == B_FALSE) {
		hasSetYuvProgram = B_TRUE;
		useYUVProgram(mYuvProgram);
	}

	if (_textureWidth != width || _textureHeight != height) {
		setupTextures(width, height);
	}

	updateTextures(data, width, height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, INDICES_YUV);
	checkGlError("glDrawArrays");

	return 0;
}

static int32_t onRenderH264(char *data, int32_t width, int32_t height, int texId) {
	if (hasSetH264Program == B_FALSE) {
		hasSetH264Program = B_TRUE;
		useH264Program(mH264Program);
	}

//	if (_textureWidth != (GLsizei) width || _textureHeight != (GLsizei) height) {
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);
//		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
//				GL_UNSIGNED_BYTE, NULL);
//		checkGlError("SetupTextures");
//		_textureWidth = width;
//		_textureHeight = height;
//	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);

//	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, DRAW_ORDER);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, INDICES_H264);
	checkGlError("glDrawArrays");

	return 0;
}

JNI(jboolean, glInit) (JNIEnv *env, jobject obj, jobject glView, jint width, jint height) {
	if (jview != NULL) {
		(*env)->DeleteGlobalRef(env, jview);
		jview = NULL;
	}
	jview = (*env)->NewGlobalRef(env, glView);
	gl_set_type(env, mRenderType);
	onSetup(width, height);
	isCreated = B_TRUE;
	return B_TRUE;
}

JNI(void, glUninit)(JNIEnv *env, jobject obj) {
	isCreated = B_FALSE;
	_textureWidth = -1;
	_textureHeight = -1;
	if (jview != NULL) {
		(*env)->DeleteGlobalRef(env, jview);
		jview = NULL;
	}
	return;
}

JNI(void, glRender)(JNIEnv *env, jobject obj, jboolean _isHwRending) {
	if (isCreated == B_FALSE) {
		return;
	}
	isHwRending = _isHwRending;
	switch (mRenderType) {
	case GL_RENDER_HW:
		onRenderH264(g_buffer, g_width, g_height, _textureIds[0]);
		break;
	case GL_RENDER_SW:
		onRenderYUV(g_buffer, g_width, g_height);
		break;
	default:
		break;
	}
	return;
}

JNI(jint, glGenTexture)(JNIEnv *env, jobject obj) {
//	GLuint texId[1];
//	glGenTextures(1, texId);
//	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId[0]);
//	glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    return texId[0];
    return _textureIds[0];
}

