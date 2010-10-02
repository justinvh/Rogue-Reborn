/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_backend.c
#include <hat/renderer/tr_local.h>

backEndData_t  *backEndData[SMP_FRAMES];
backEndState_t  backEnd;

void GL_Bind(image_t * image)
{
	int             texnum;

	if(!image)
	{
		ri.Printf(PRINT_WARNING, "GL_Bind: NULL image\n");
		image = tr.defaultImage;
	}
	else
	{
		if(r_logFile->integer)
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment(va("--- GL_Bind( %s ) ---\n", image->name));
		}
	}

	texnum = image->texnum;

	if(r_nobind->integer && tr.blackImage)
	{
		// performance evaluation option
		texnum = tr.blackImage->texnum;
	}

	if(glState.currenttextures[glState.currenttmu] != texnum)
	{
		image->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture(image->type, texnum);
	}
}

void GL_Unbind()
{
	GLimp_LogComment("--- GL_Unbind() ---\n");

	qglBindTexture(GL_TEXTURE_2D, 0);
}

void BindAnimatedImage(textureBundle_t * bundle)
{
	int             index;

	if(bundle->isVideoMap)
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if(bundle->numImages <= 1)
	{
		GL_Bind(bundle->image[0]);
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = Q_ftol(backEnd.refdef.floatTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if(index < 0)
	{
		index = 0;				// may happen with shader time offsets
	}
	index %= bundle->numImages;

	GL_Bind(bundle->image[index]);
}

void GL_TextureFilter(image_t * image, filterType_t filterType)
{
	if(!image)
	{
		ri.Printf(PRINT_WARNING, "GL_TextureFilter: NULL image\n");
	}
	else
	{
		if(r_logFile->integer)
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment(va("--- GL_TextureFilter( %s ) ---\n", image->name));
		}
	}

	if(image->filterType == filterType)
		return;

	// set filter type
	switch (image->filterType)
	{
			/*
			   case FT_DEFAULT:
			   qglTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			   qglTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			   // set texture anisotropy
			   if(glConfig.textureAnisotropyAvailable)
			   qglTexParameterf(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
			   break;
			 */

		case FT_LINEAR:
			qglTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;

		case FT_NEAREST:
			qglTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;

		default:
			break;
	}
}

void GL_BindProgram(shaderProgram_t * program)
{
	if(!program)
	{
		GL_BindNullProgram();
		return;
	}

	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- GL_BindProgram( %s ) ---\n", program->name));
	}

	if(glState.currentProgram != program)
	{
		qglUseProgramObjectARB(program->program);
		glState.currentProgram = program;
	}
}

void GL_BindNullProgram(void)
{
	if(r_logFile->integer)
	{
		GLimp_LogComment("--- GL_BindNullProgram ---\n");
	}

	if(glState.currentProgram)
	{
		qglUseProgramObjectARB(0);
		glState.currentProgram = NULL;
	}
}

void GL_SelectTexture(int unit)
{
	if(glState.currenttmu == unit)
	{
		return;
	}

	if(unit >= 0 && unit <= 31)
	{
		qglActiveTextureARB(GL_TEXTURE0_ARB + unit);

		if(r_logFile->integer)
		{
			GLimp_LogComment(va("glActiveTextureARB( GL_TEXTURE%i_ARB )\n", unit));
		}
	}
	else
	{
		ri.Error(ERR_DROP, "GL_SelectTexture: unit = %i", unit);
	}

	glState.currenttmu = unit;
}

void GL_BlendFunc(GLenum sfactor, GLenum dfactor)
{
	if(glState.blendSrc != sfactor || glState.blendDst != dfactor)
	{
		glState.blendSrc = sfactor;
		glState.blendDst = dfactor;

		qglBlendFunc(sfactor, dfactor);
	}
}

void GL_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	if(glState.clearColorRed != red || glState.clearColorGreen != green || glState.clearColorBlue != blue || glState.clearColorAlpha != alpha)
	{
		glState.clearColorRed = red;
		glState.clearColorGreen = green;
		glState.clearColorBlue = blue;
		glState.clearColorAlpha = alpha;

		qglClearColor(red, green, blue, alpha);
	}
}

void GL_ClearDepth(GLclampd depth)
{
	if(glState.clearDepth != depth)
	{
		glState.clearDepth = depth;

		qglClearDepth(depth);
	}
}

void GL_ClearStencil(GLint s)
{
	if(glState.clearStencil != s)
	{
		glState.clearStencil = s;

		qglClearStencil(s);
	}
}

void GL_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	if(glState.colorMaskRed != red || glState.colorMaskGreen != green || glState.colorMaskBlue != blue || glState.colorMaskAlpha != alpha)
	{
		glState.colorMaskRed = red;
		glState.colorMaskGreen = green;
		glState.colorMaskBlue = blue;
		glState.colorMaskAlpha = alpha;

		qglColorMask(red, green, blue, alpha);
	}
}

void GL_CullFace(GLenum mode)
{
	if(glState.cullFace != mode)
	{
		glState.cullFace = mode;

		qglCullFace(mode);
	}
}

void GL_DepthFunc(GLenum func)
{
	if(glState.depthFunc != func)
	{
		glState.depthFunc = func;

		qglDepthFunc(func);
	}
}

void GL_DepthMask(GLboolean flag)
{
	if(glState.depthMask != flag)
	{
		glState.depthMask = flag;

		qglDepthMask(flag);
	}
}

void GL_DrawBuffer(GLenum mode)
{
	if(glState.drawBuffer != mode)
	{
		glState.drawBuffer = mode;

		qglDrawBuffer(mode);
	}
}

void GL_FrontFace(GLenum mode)
{
	if(glState.frontFace != mode)
	{
		glState.frontFace = mode;

		qglFrontFace(mode);
	}
}

void GL_LoadModelViewMatrix(const matrix_t m)
{
#if 1
	if(MatrixCompare(glState.modelViewMatrix[glState.stackIndex], m))
	{
		return;
	}
#endif


	MatrixCopy(m, glState.modelViewMatrix[glState.stackIndex]);
	MatrixMultiply(glState.projectionMatrix[glState.stackIndex], glState.modelViewMatrix[glState.stackIndex],
				   glState.modelViewProjectionMatrix[glState.stackIndex]);
}

void GL_LoadProjectionMatrix(const matrix_t m)
{
#if 1
	if(MatrixCompare(glState.projectionMatrix[glState.stackIndex], m))
	{
		return;
	}
#endif

	MatrixCopy(m, glState.projectionMatrix[glState.stackIndex]);
	MatrixMultiply(glState.projectionMatrix[glState.stackIndex], glState.modelViewMatrix[glState.stackIndex],
				   glState.modelViewProjectionMatrix[glState.stackIndex]);
}

void GL_PushMatrix()
{
	glState.stackIndex++;

	if(glState.stackIndex >= MAX_GLSTACK)
	{
		glState.stackIndex = MAX_GLSTACK - 1;
		ri.Error(ERR_DROP, "GL_PushMatrix: stack overflow = %i", glState.stackIndex);
	}
}

void GL_PopMatrix()
{
	glState.stackIndex--;

	if(glState.stackIndex < 0)
	{
		glState.stackIndex = 0;
		ri.Error(ERR_DROP, "GL_PushMatrix: stack underflow");
	}
}

void GL_PolygonMode(GLenum face, GLenum mode)
{
	if(glState.polygonFace != face || glState.polygonMode != mode)
	{
		glState.polygonFace = face;
		glState.polygonMode = mode;

		qglPolygonMode(face, mode);
	}
}

void GL_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(glState.scissorX != x || glState.scissorY != y || glState.scissorWidth != width || glState.scissorHeight != height)
	{
		glState.scissorX = x;
		glState.scissorY = y;
		glState.scissorWidth = width;
		glState.scissorHeight = height;

		qglScissor(x, y, width, height);
	}
}

void GL_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(glState.viewportX != x || glState.viewportY != y || glState.viewportWidth != width || glState.viewportHeight != height)
	{
		glState.viewportX = x;
		glState.viewportY = y;
		glState.viewportWidth = width;
		glState.viewportHeight = height;

		qglViewport(x, y, width, height);
	}
}

void GL_Cull(int cullType)
{
	if(glState.faceCulling == cullType)
	{
		return;
	}

#if 1
	glState.faceCulling = cullType;

	if(cullType == CT_TWO_SIDED)
	{
		qglDisable(GL_CULL_FACE);
	}
	else
	{
		qglEnable(GL_CULL_FACE);

		if(cullType == CT_BACK_SIDED)
		{
			GL_CullFace(GL_BACK);

			if(backEnd.viewParms.isMirror)
			{
				GL_FrontFace(GL_CW);
			}
			else
			{
				GL_FrontFace(GL_CCW);
			}
		}
		else
		{
			GL_CullFace(GL_FRONT);

			if(backEnd.viewParms.isMirror)
			{
				GL_FrontFace(GL_CW);
			}
			else
			{
				GL_FrontFace(GL_CCW);
			}
		}
	}
#else
	glState.faceCulling = CT_TWO_SIDED;
	qglDisable(GL_CULL_FACE);
#endif
}


/*
GL_State

This routine is responsible for setting the most commonly changed state
in Q3.
*/
void GL_State(uint32_t stateBits)
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if(!diff)
	{
		return;
	}

	// check depthFunc bits
	if(diff & GLS_DEPTHFUNC_BITS)
	{
		switch (stateBits & GLS_DEPTHFUNC_BITS)
		{
			default:
				GL_DepthFunc(GL_LEQUAL);
				break;
			case GLS_DEPTHFUNC_LESS:
				GL_DepthFunc(GL_LESS);
				break;
			case GLS_DEPTHFUNC_EQUAL:
				GL_DepthFunc(GL_EQUAL);
				break;
		}
	}

	// check blend bits
	if(diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
	{
		GLenum          srcFactor, dstFactor;

		if(stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			switch (stateBits & GLS_SRCBLEND_BITS)
			{
				case GLS_SRCBLEND_ZERO:
					srcFactor = GL_ZERO;
					break;
				case GLS_SRCBLEND_ONE:
					srcFactor = GL_ONE;
					break;
				case GLS_SRCBLEND_DST_COLOR:
					srcFactor = GL_DST_COLOR;
					break;
				case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
					srcFactor = GL_ONE_MINUS_DST_COLOR;
					break;
				case GLS_SRCBLEND_SRC_ALPHA:
					srcFactor = GL_SRC_ALPHA;
					break;
				case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
					srcFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case GLS_SRCBLEND_DST_ALPHA:
					srcFactor = GL_DST_ALPHA;
					break;
				case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
					srcFactor = GL_ONE_MINUS_DST_ALPHA;
					break;
				case GLS_SRCBLEND_ALPHA_SATURATE:
					srcFactor = GL_SRC_ALPHA_SATURATE;
					break;
				default:
					srcFactor = GL_ONE;	// to get warning to shut up
					ri.Error(ERR_DROP, "GL_State: invalid src blend state bits\n");
					break;
			}

			switch (stateBits & GLS_DSTBLEND_BITS)
			{
				case GLS_DSTBLEND_ZERO:
					dstFactor = GL_ZERO;
					break;
				case GLS_DSTBLEND_ONE:
					dstFactor = GL_ONE;
					break;
				case GLS_DSTBLEND_SRC_COLOR:
					dstFactor = GL_SRC_COLOR;
					break;
				case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
					dstFactor = GL_ONE_MINUS_SRC_COLOR;
					break;
				case GLS_DSTBLEND_SRC_ALPHA:
					dstFactor = GL_SRC_ALPHA;
					break;
				case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case GLS_DSTBLEND_DST_ALPHA:
					dstFactor = GL_DST_ALPHA;
					break;
				case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
					dstFactor = GL_ONE_MINUS_DST_ALPHA;
					break;
				default:
					dstFactor = GL_ONE;	// to get warning to shut up
					ri.Error(ERR_DROP, "GL_State: invalid dst blend state bits\n");
					break;
			}

			qglEnable(GL_BLEND);
			GL_BlendFunc(srcFactor, dstFactor);
		}
		else
		{
			qglDisable(GL_BLEND);
		}
	}

	// check colormask
	if(diff & GLS_COLORMASK_BITS)
	{
		if(stateBits & GLS_COLORMASK_BITS)
		{
			GL_ColorMask((stateBits & GLS_REDMASK_FALSE) ? GL_FALSE : GL_TRUE,
						 (stateBits & GLS_GREENMASK_FALSE) ? GL_FALSE : GL_TRUE,
						 (stateBits & GLS_BLUEMASK_FALSE) ? GL_FALSE : GL_TRUE,
						 (stateBits & GLS_ALPHAMASK_FALSE) ? GL_FALSE : GL_TRUE);
		}
		else
		{
			GL_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
	}

	// check depthmask
	if(diff & GLS_DEPTHMASK_TRUE)
	{
		if(stateBits & GLS_DEPTHMASK_TRUE)
		{
			GL_DepthMask(GL_TRUE);
		}
		else
		{
			GL_DepthMask(GL_FALSE);
		}
	}

	// fill/line mode
	if(diff & GLS_POLYMODE_LINE)
	{
		if(stateBits & GLS_POLYMODE_LINE)
		{
			GL_PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			GL_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	// depthtest
	if(diff & GLS_DEPTHTEST_DISABLE)
	{
		if(stateBits & GLS_DEPTHTEST_DISABLE)
		{
			qglDisable(GL_DEPTH_TEST);
		}
		else
		{
			qglEnable(GL_DEPTH_TEST);
		}
	}

	// alpha test - deprecated in OpenGL 3.0
	/*
	   if(diff & GLS_ATEST_BITS)
	   {
	   switch (stateBits & GLS_ATEST_BITS)
	   {
	   case 0:
	   qglDisable(GL_ALPHA_TEST);
	   break;
	   case GLS_ATEST_GT_0:
	   qglEnable(GL_ALPHA_TEST);
	   qglAlphaFunc(GL_GREATER, 0.0f);
	   break;
	   case GLS_ATEST_LT_80:
	   qglEnable(GL_ALPHA_TEST);
	   qglAlphaFunc(GL_LESS, 0.5f);
	   break;
	   case GLS_ATEST_GE_80:
	   qglEnable(GL_ALPHA_TEST);
	   qglAlphaFunc(GL_GEQUAL, 0.5f);
	   break;
	   case GLS_ATEST_GT_CUSTOM:
	   // FIXME
	   qglEnable(GL_ALPHA_TEST);
	   qglAlphaFunc(GL_GREATER, 0.5f);
	   break;
	   default:
	   assert(0);
	   break;
	   }
	   }
	 */

	// stenciltest
	if(diff & GLS_STENCILTEST_ENABLE)
	{
		if(stateBits & GLS_STENCILTEST_ENABLE)
		{
			qglEnable(GL_STENCIL_TEST);
		}
		else
		{
			qglDisable(GL_STENCIL_TEST);
		}
	}

	glState.glStateBits = stateBits;
}


void GL_VertexAttribsState(uint32_t stateBits)
{
	uint32_t		diff;

	if(glConfig.vboVertexSkinningAvailable && tess.vboVertexSkinning)
		stateBits |= (ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);

	GL_VertexAttribPointers(stateBits);

	diff = stateBits ^ glState.vertexAttribsState;
	if(!diff)
	{
		return;
	}

	if(diff & ATTR_POSITION)
	{
		if(stateBits & ATTR_POSITION)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_POSITION )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_POSITION);
		}
	}

	if(diff & ATTR_TEXCOORD)
	{
		if(stateBits & ATTR_TEXCOORD)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TEXCOORD )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
		}
	}

	if(diff & ATTR_LIGHTCOORD)
	{
		if(stateBits & ATTR_LIGHTCOORD)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTCOORD )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
		}
	}

	if(diff & ATTR_TANGENT)
	{
		if(stateBits & ATTR_TANGENT)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_TANGENT )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
		}
	}

	if(diff & ATTR_BINORMAL)
	{
		if(stateBits & ATTR_BINORMAL)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BINORMAL )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BINORMAL);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BINORMAL )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BINORMAL);
		}
	}

	if(diff & ATTR_NORMAL)
	{
		if(stateBits & ATTR_NORMAL)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_NORMAL )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_NORMAL);
		}
	}

	if(diff & ATTR_COLOR)
	{
		if(stateBits & ATTR_COLOR)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_COLOR )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_COLOR);
		}
	}

	if(diff & ATTR_PAINTCOLOR)
	{
		if(stateBits & ATTR_PAINTCOLOR)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_PAINTCOLOR )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_PAINTCOLOR);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_PAINTCOLOR )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_PAINTCOLOR);
		}
	}

	if(diff & ATTR_LIGHTDIRECTION)
	{
		if(stateBits & ATTR_LIGHTDIRECTION)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_LIGHTDIRECTION )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_LIGHTDIRECTION);
		}
	}

	if(diff & ATTR_BONE_INDEXES)
	{
		if(stateBits & ATTR_BONE_INDEXES)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BONE_INDEXES )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BONE_INDEXES);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BONE_INDEXES )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BONE_INDEXES);
		}
	}

	if(diff & ATTR_BONE_WEIGHTS)
	{
		if(stateBits & ATTR_BONE_WEIGHTS)
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglEnableVertexAttribArrayARB( ATTR_INDEX_BONE_WEIGHTS )\n");
			}
			qglEnableVertexAttribArrayARB(ATTR_INDEX_BONE_WEIGHTS);
		}
		else
		{
			if(r_logFile->integer)
			{
				GLimp_LogComment("qglDisableVertexAttribArrayARB( ATTR_INDEX_BONE_WEIGHTS )\n");
			}
			qglDisableVertexAttribArrayARB(ATTR_INDEX_BONE_WEIGHTS);
		}
	}

	glState.vertexAttribsState = stateBits;
}

void GL_VertexAttribPointers(uint32_t attribBits)
{
	if(!glState.currentVBO)
	{
		ri.Error(ERR_FATAL, "GL_VertexAttribPointers: no VBO bound");
		return;
	}

	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- GL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name));
	}

	if(glConfig.vboVertexSkinningAvailable && tess.vboVertexSkinning)
		attribBits |= (ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS);

	if((attribBits & ATTR_POSITION) && !(glState.vertexAttribPointersSet & ATTR_POSITION))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_POSITION )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_POSITION, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsXYZ));
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if((attribBits & ATTR_TEXCOORD) && !(glState.vertexAttribPointersSet & ATTR_TEXCOORD))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TEXCOORD )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD0, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsTexCoords));
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if((attribBits & ATTR_LIGHTCOORD) && !(glState.vertexAttribPointersSet & ATTR_LIGHTCOORD))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTCOORD )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD1, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsLightCoords));
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if((attribBits & ATTR_TANGENT) && !(glState.vertexAttribPointersSet & ATTR_TANGENT))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_TANGENT )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsTangents));
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if((attribBits & ATTR_BINORMAL) && !(glState.vertexAttribPointersSet & ATTR_BINORMAL))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BINORMAL )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_BINORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsBinormals));
		glState.vertexAttribPointersSet |= ATTR_BINORMAL;
	}

	if((attribBits & ATTR_NORMAL) && !(glState.vertexAttribPointersSet & ATTR_NORMAL))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_NORMAL )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsNormals));
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

	if((attribBits & ATTR_COLOR) && !(glState.vertexAttribPointersSet & ATTR_COLOR))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_COLOR )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsColors));
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

	if((attribBits & ATTR_PAINTCOLOR) && !(glState.vertexAttribPointersSet & ATTR_PAINTCOLOR))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_PAINTCOLOR )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_PAINTCOLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsPaintColors));
		glState.vertexAttribPointersSet |= ATTR_PAINTCOLOR;
	}

	if((attribBits & ATTR_LIGHTDIRECTION) && !(glState.vertexAttribPointersSet & ATTR_LIGHTDIRECTION))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_LIGHTDIRECTION )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_LIGHTDIRECTION, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET(glState.currentVBO->ofsLightDirections));
		glState.vertexAttribPointersSet |= ATTR_LIGHTDIRECTION;
	}

	if((attribBits & ATTR_BONE_INDEXES) && !(glState.vertexAttribPointersSet & ATTR_BONE_INDEXES))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BONE_INDEXES )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_BONE_INDEXES, 4, GL_INT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneIndexes));
		glState.vertexAttribPointersSet |= ATTR_BONE_INDEXES;
	}

	if((attribBits & ATTR_BONE_WEIGHTS) && !(glState.vertexAttribPointersSet & ATTR_BONE_WEIGHTS))
	{
		if(r_logFile->integer)
		{
			GLimp_LogComment("qglVertexAttribPointerARB( ATTR_INDEX_BONE_WEIGHTS )\n");
		}

		qglVertexAttribPointerARB(ATTR_INDEX_BONE_WEIGHTS, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET(glState.currentVBO->ofsBoneWeights));
		glState.vertexAttribPointersSet |= ATTR_BONE_WEIGHTS;
	}
}




/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace(void)
{
	float           c;

	if(!backEnd.isHyperspace)
	{
		// do initialization shit
	}

	c = (backEnd.refdef.time & 255) / 255.0f;
	GL_ClearColor(c, c, c, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor(void)
{
	GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}




/*
================
RB_SetGL2D
================
*/
static void RB_SetGL2D(void)
{
	matrix_t        proj;

	GLimp_LogComment("--- RB_SetGL2D ---\n");

	// disable offscreen rendering
	if(glConfig.framebufferObjectAvailable)
	{
		R_BindNullFBO();
	}

	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	GL_Viewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	GL_Scissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	MatrixOrthogonalProjection(proj, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	GL_LoadProjectionMatrix(proj);
	GL_LoadModelViewMatrix(matrixIdentity);

	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	GL_Cull(CT_TWO_SIDED);
	qglDisable(GL_CLIP_PLANE0);

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}



static void RB_RenderDrawSurfaces(qboolean opaque, qboolean depthFill)
{
	trRefEntity_t  *entity, *oldEntity;
	shader_t       *shader, *oldShader;
	int             lightmapNum, oldLightmapNum;
	qboolean        depthRange, oldDepthRange;
	int             i;
	drawSurf_t     *drawSurf;

	GLimp_LogComment("--- RB_RenderDrawSurfaces ---\n");

	// draw everything
	oldEntity = NULL;
	oldShader = NULL;
	oldLightmapNum = -1;
	oldDepthRange = qfalse;
	depthRange = qfalse;
	backEnd.currentLight = NULL;

	for(i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++)
	{
		// update locals
		entity = drawSurf->entity;
		shader = tr.sortedShaders[drawSurf->shaderNum];
		lightmapNum = drawSurf->lightmapNum;

		if(opaque)
		{
			// skip all translucent surfaces that don't matter for this pass
			if(shader->sort > SS_OPAQUE)
			{
				break;
			}
		}
		else
		{
			// skip all opaque surfaces that don't matter for this pass
			if(shader->sort <= SS_OPAQUE)
			{
				continue;
			}
		}

		if(entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum)
		{
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
			continue;
		}

		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if(shader != oldShader || lightmapNum != oldLightmapNum || (entity != oldEntity && !shader->entityMergable))
		{
			if(oldShader != NULL)
			{
				Tess_End();
			}

			if(depthFill)
				Tess_Begin(Tess_StageIteratorDepthFill, shader, NULL, qtrue, qfalse, lightmapNum);
			else
				Tess_Begin(Tess_StageIteratorGeneric, shader, NULL, qfalse, qfalse, lightmapNum);

			oldShader = shader;
			oldLightmapNum = lightmapNum;
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				backEnd.currentEntity = entity;

				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
	}

	// draw the contents of the last shader batch
	if(oldShader != NULL)
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	GL_CheckErrors();
}

// *INDENT-OFF*
#ifdef VOLUMETRIC_LIGHTING
static void Render_lightVolume(interaction_t * ia)
{
	int             j;
	trRefLight_t   *light;
	shader_t       *lightShader;
	shaderStage_t  *attenuationXYStage;
	shaderStage_t  *attenuationZStage;
	matrix_t        ortho;
	vec4_t          quadVerts[4];

	light = ia->light;

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// set light scissor to reduce fillrate
	GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY,
									backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	switch (light->l.rlType)
	{
		case RL_PROJ:
		{
			MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
			MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));	// scale
			break;
		}

		case RL_OMNI:
		default:
		{
			MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
			MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
			break;
		}
	}
	MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
	MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);

	lightShader = light->shader;
	attenuationZStage = lightShader->stages[0];

	for(j = 1; j < MAX_SHADER_STAGES; j++)
	{
		attenuationXYStage = lightShader->stages[j];

		if(!attenuationXYStage)
		{
			break;
		}

		if(attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
		{
			continue;
		}

		if(!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
		{
			continue;
		}

		Tess_ComputeColor(attenuationXYStage);
		R_ComputeFinalAttenuation(attenuationXYStage, light);

		if(light->l.rlType == RL_OMNI)
		{
			vec3_t          viewOrigin;
			vec3_t          lightOrigin;
			vec4_t          lightColor;
			qboolean		shadowCompare;

			GLimp_LogComment("--- Render_lightVolume_omni ---\n");

			// enable shader, set arrays
			GL_BindProgram(&tr.lightVolumeShader_omni);
			//GL_VertexAttribsState(tr.lightVolumeShader_omni.attribs);
			GL_Cull(CT_TWO_SIDED);
			GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
			//GL_State(GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
			//GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			//GL_State(attenuationXYStage->stateBits & ~(GLS_DEPTHMASK_TRUE | GLS_DEPTHTEST_DISABLE));

			// set uniforms
			VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);	// in world space
			VectorCopy(light->origin, lightOrigin);
			VectorCopy(tess.svars.color, lightColor);

			shadowCompare = r_shadows->integer >= 4 && !light->l.noShadows && light->shadowLOD >= 0;

			GLSL_SetUniform_ViewOrigin(&tr.lightVolumeShader_omni, viewOrigin);
			GLSL_SetUniform_LightOrigin(&tr.lightVolumeShader_omni, lightOrigin);
			GLSL_SetUniform_LightColor(&tr.lightVolumeShader_omni, lightColor);
			GLSL_SetUniform_LightRadius(&tr.lightVolumeShader_omni, light->sphereRadius);
			GLSL_SetUniform_LightScale(&tr.lightVolumeShader_omni, light->l.scale);
			GLSL_SetUniform_LightAttenuationMatrix(&tr.lightVolumeShader_omni, light->attenuationMatrix2);

			// FIXME GLSL_SetUniform_ShadowMatrix(&tr.lightVolumeShader_omni, light->attenuationMatrix);
			GLSL_SetUniform_ShadowCompare(&tr.lightVolumeShader_omni, shadowCompare);

			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.lightVolumeShader_omni, glState.modelViewProjectionMatrix[glState.stackIndex]);
			GLSL_SetUniform_UnprojectMatrix(&tr.lightVolumeShader_omni, backEnd.viewParms.unprojectionMatrix);

			//GLSL_SetUniform_PortalClipping(&tr.lightVolumeShader_omni, backEnd.viewParms.isPortal);

			// bind u_DepthMap
			GL_SelectTexture(0);
			if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
					   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
			{
				GL_Bind(tr.depthRenderImage);
			}
			else if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
			{
				GL_Bind(tr.depthRenderImage);
			}
			else
			{
				// depth texture is not bound to a FBO
				GL_Bind(tr.depthRenderImage);
				qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
			}

			// bind u_AttenuationMapXY
			GL_SelectTexture(1);
			BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

			// bind u_AttenuationMapZ
			GL_SelectTexture(2);
			BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

			// bind u_ShadowMap
			if(shadowCompare)
			{
				GL_SelectTexture(3);
				GL_Bind(tr.shadowCubeFBOImage[light->shadowLOD]);
			}

			// draw light scissor rectangle
			VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
			VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
			VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0,
					   1);
			VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
			Tess_InstantQuad(quadVerts);

			GL_CheckErrors();
		}
	}

	GL_PopMatrix();
}
#endif
// *INDENT-ON*


/*
 * helper function for parallel split shadow mapping
 */
static int MergeInteractionBounds(const matrix_t lightViewProjectionMatrix, interaction_t * ia, int iaCount, vec3_t bounds[2], qboolean shadowCasters)
{
	//int				i;
	int				j;
	surfaceType_t  *surface;
	vec4_t			point;
	vec4_t			transf;
	vec3_t			worldBounds[2];
	//vec3_t		viewBounds[2];
	//vec3_t		center;
	//float			radius;
	int				numCasters;

	frustum_t       frustum;
	//cplane_t       *clipPlane;
	//int             r;

	numCasters = 0;
	ClearBounds(bounds[0], bounds[1]);

	// calculate frustum planes using the modelview projection matrix
	R_SetupFrustum2(frustum, lightViewProjectionMatrix);

	while(iaCount < backEnd.viewParms.numInteractions)
	{
		surface = ia->surface;

		if(shadowCasters)
		{
			if(ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}
		}
		else
		{
			// we only merge shadow receivers
			if(ia->type == IA_SHADOWONLY)
			{
				goto skipInteraction;
			}
		}

		if(*surface == SF_FACE)
		{
			srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface;

			VectorCopy(face->bounds[0], worldBounds[0]);
			VectorCopy(face->bounds[1], worldBounds[1]);
		}
		else if(*surface == SF_GRID)
		{
			srfGridMesh_t  *grid = (srfGridMesh_t *) surface;

			VectorCopy(grid->meshBounds[0], worldBounds[0]);
			VectorCopy(grid->meshBounds[1], worldBounds[1]);
		}
		else if(*surface == SF_TRIANGLES)
		{
			srfTriangles_t *tri = (srfTriangles_t *) surface;

			//ri.Printf(PRINT_ALL, "merging triangle bounds\n");

			VectorCopy(tri->bounds[0], worldBounds[0]);
			VectorCopy(tri->bounds[1], worldBounds[1]);
		}
		else if(*surface == SF_VBO_MESH)
		{
			srfVBOMesh_t   *srf = (srfVBOMesh_t *) surface;

			//ri.Printf(PRINT_ALL, "merging vbo mesh bounds\n");

			VectorCopy(srf->bounds[0], worldBounds[0]);
			VectorCopy(srf->bounds[1], worldBounds[1]);
		}
		else if(*surface == SF_MDX)
		{
			//Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], lightColor);
		}

#if 0
		// use the frustum planes to cut off shadow casters beyond the split frustum
		for(i = 0; i < 4; i++)
		{
			clipPlane = &frustum[i];

			r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], clipPlane);
			if(r == 2)
			{
				goto skipInteraction;
			}
		}
#endif

		if(shadowCasters && ia->type != IA_LIGHTONLY)
		{
			numCasters++;
		}

#if 1
		for(j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			MatrixTransform4(lightViewProjectionMatrix, point, transf);
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			AddPointToBounds(transf, bounds[0], bounds[1]);
		}
#elif 0
		ClearBounds(viewBounds[0], viewBounds[1]);
		for(j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			MatrixTransform4(lightViewProjectionMatrix, point, transf);
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];

			AddPointToBounds(transf, viewBounds[0], viewBounds[1]);
		}

		// get sphere of AABB
		VectorAdd(viewBounds[0], viewBounds[1], center);
		VectorScale(center, 0.5, center);

		radius = RadiusFromBounds(viewBounds[0], viewBounds[1]);

		for(j = 0; j < 3; j++)
		{
			if((transf[j] - radius) < bounds[0][j])
			{
				bounds[0][j] = transf[i] - radius;
			}

			if((transf[j] + radius) > bounds[1][j])
			{
				bounds[1][j] = transf[i] + radius;
			}
		}
#else

		ClearBounds(viewBounds[0], viewBounds[1]);
		for(j = 0; j < 8; j++)
		{
			point[0] = worldBounds[j & 1][0];
			point[1] = worldBounds[(j >> 1) & 1][1];
			point[2] = worldBounds[(j >> 2) & 1][2];
			point[3] = 1;

			MatrixTransform4(lightViewProjectionMatrix, point, transf);
			//transf[0] /= transf[3];
			//transf[1] /= transf[3];
			//transf[2] /= transf[3];

			AddPointToBounds(transf, viewBounds[0], viewBounds[1]);
		}

		// get sphere of AABB
		VectorAdd(viewBounds[0], viewBounds[1], center);
		VectorScale(center, 0.5, center);

		//MatrixTransform4(lightViewProjectionMatrix, center, transf);
		//transf[0] /= transf[3];
		//transf[1] /= transf[3];
		//transf[2] /= transf[3];

		radius = RadiusFromBounds(viewBounds[0], viewBounds[1]);

		if((transf[2] + radius) > bounds[1][2])
		{
			bounds[1][2] = transf[2] + radius;
		}
#endif

	skipInteraction:
		if(!ia->next)
		{
			// this is the last interaction of the current light
			break;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	return numCasters;
}



/*
=================
RB_RenderInteractions
=================
*/
static void RB_RenderInteractions()
{
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	interaction_t  *ia;
	qboolean        depthRange, oldDepthRange;
	int             iaCount;
	surfaceType_t  *surface;
	vec3_t          tmp;
	matrix_t        modelToLight;
	int             startTime = 0, endTime = 0;

	GLimp_LogComment("--- RB_RenderInteractions ---\n");

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = qfalse;
	depthRange = qfalse;

	// render interactions
	for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		if(!shader->interactLight)
		{
			// skip this interaction because the surface shader has no ability to interact with light
			// this will save texcoords and matrix calculations
			goto skipInteraction;
		}

		if(ia->type == IA_SHADOWONLY)
		{
			// skip this interaction because the interaction is meant for shadowing only
			goto skipInteraction;
		}

		if(light != oldLight)
		{
			GLimp_LogComment("----- Rendering new light -----\n");

			// set light scissor to reduce fillrate
			GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);
		}

		// Tr3B: this should never happen in the first iteration
		if(light == oldLight && entity == oldEntity && shader == oldShader)
		{
			// fast path, same as previous
			rb_surfaceTable[*surface] (surface);
			goto nextInteraction;
		}

		// draw the contents of the last shader batch
		Tess_End();

		// begin a new batch
		Tess_Begin(Tess_StageIteratorLighting, shader, light->shader, qfalse, qfalse, -1);

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if(light != oldLight || entity != oldEntity)
		{
			// transform light origin into model space for u_LightOrigin parameter
			if(entity != &tr.worldEntity)
			{
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
			}
			else
			{
				VectorCopy(light->origin, light->transformed);
			}

			// build the attenuation matrix using the entity transform
			MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight);

			switch (light->l.rlType)
			{
				case RL_PROJ:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));	// scale
					break;
				}

				case RL_OMNI:
				default:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
					break;
				}
			}
			MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
			MatrixMultiply2(light->attenuationMatrix, modelToLight);
		}

		// add the triangles for this surface
		rb_surfaceTable[*surface] (surface);

	  nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;

	  skipInteraction:
		if(!ia->next)
		{
			// draw the contents of the last shader batch
			Tess_End();

#ifdef VOLUMETRIC_LIGHTING
			// draw the light volume if needed
			if(light->shader->volumetricLight)
			{
				Render_lightVolume(ia);
			}
#endif

			if(iaCount < (backEnd.viewParms.numInteractions - 1))
			{
				// jump to next interaction and continue
				ia++;
				iaCount++;
			}
			else
			{
				// increase last time to leave for loop
				iaCount++;
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// reset scissor
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_forwardLightingTime = endTime - startTime;
	}
}

/*
=================
RB_RenderInteractionsStencilShadowed
=================
*/
static void RB_RenderInteractionsStencilShadowed()
{
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	interaction_t  *ia;
	int             iaCount;
	int             iaFirst;
	surfaceType_t  *surface;
	qboolean        depthRange, oldDepthRange;
	vec3_t          tmp;
	matrix_t        modelToLight;
	qboolean        drawShadows;
	int             startTime = 0, endTime = 0;

	if(glConfig.stencilBits < 4)
	{
		RB_RenderInteractions();
		return;
	}

	GLimp_LogComment("--- RB_RenderInteractionsStencilShadowed ---\n");

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = qfalse;
	depthRange = qfalse;
	drawShadows = qtrue;

	/*
	   if(qglActiveStencilFaceEXT)
	   {
	   qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	   }
	 */

	// render interactions
	for(iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if(iaCount == iaFirst)
		{
			if(r_logFile->integer)
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment(va("----- First Interaction: %i -----\n", iaCount));
			}

			if(drawShadows)
			{
				// set light scissor to reduce fillrate
				GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

				// set depth test to reduce fillrate
				if(qglDepthBoundsEXT)
				{
					if(!ia->noDepthBoundsTest)
					{
						qglEnable(GL_DEPTH_BOUNDS_TEST_EXT);
						qglDepthBoundsEXT(ia->depthNear, ia->depthFar);
					}
					else
					{
						qglDisable(GL_DEPTH_BOUNDS_TEST_EXT);
					}
				}

				if(!light->l.noShadows)
				{
					GLimp_LogComment("--- Rendering shadow volumes ---\n");

					// set the reference stencil value
					GL_ClearStencil(128);

					// reset stencil buffer
					qglClear(GL_STENCIL_BUFFER_BIT);

					// use less compare as depthfunc
					// don't write to the color buffer or depth buffer
					// enable stencil testing for this light
					GL_State(GLS_DEPTHFUNC_LESS | GLS_COLORMASK_BITS | GLS_STENCILTEST_ENABLE);

					qglStencilFunc(GL_ALWAYS, 128, 255);
					qglStencilMask(255);

					qglEnable(GL_POLYGON_OFFSET_FILL);
					qglPolygonOffset(r_shadowOffsetFactor->value, r_shadowOffsetUnits->value);

					// enable shadow volume extrusion shader
					GL_BindProgram(&tr.shadowExtrudeShader);
				}
			}
			else
			{
				GLimp_LogComment("--- Rendering lighting ---\n");

				if(!light->l.noShadows)
				{
					qglStencilFunc(GL_EQUAL, 128, 255);
				}
				else
				{
					// don't consider shadow volumes
					qglStencilFunc(GL_ALWAYS, 128, 255);
				}

				/*
				   if(qglActiveStencilFaceEXT)
				   {
				   qglActiveStencilFaceEXT(GL_BACK);
				   qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

				   qglActiveStencilFaceEXT(GL_FRONT);
				   qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				   }
				   else
				 */
				{
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}

				//qglDisable(GL_POLYGON_OFFSET_FILL);

				// disable shadow volume extrusion shader
				GL_BindProgram(NULL);
			}
		}

		if(drawShadows)
		{
			if(entity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			{
				goto skipInteraction;
			}

			if(shader->sort > SS_OPAQUE)
			{
				goto skipInteraction;
			}

			if(shader->noShadows || light->l.noShadows)
			{
				goto skipInteraction;
			}

			if(ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}

			if(light == oldLight && entity == oldEntity)
			{
				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Batching Shadow Interaction: %i -----\n", iaCount));
				}

				// fast path, same as previous
				rb_surfaceTable[*surface] (surface);
				goto nextInteraction;
			}
			else
			{
				if(oldLight)
				{
					// draw the contents of the last shader batch
					Tess_End();
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Beginning Shadow Interaction: %i -----\n", iaCount));
				}

				// we don't need tangent space calculations here
				Tess_Begin(Tess_StageIteratorStencilShadowVolume, shader, light->shader, qtrue, qtrue, -1);
			}
		}
		else
		{
			if(!shader->interactLight)
			{
				goto skipInteraction;
			}

			if(ia->type == IA_SHADOWONLY)
			{
				goto skipInteraction;
			}

			if(light == oldLight && entity == oldEntity && shader == oldShader)
			{
				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Batching Light Interaction: %i -----\n", iaCount));
				}

				// fast path, same as previous
				rb_surfaceTable[*surface] (surface);
				goto nextInteraction;
			}
			else
			{
				if(oldLight)
				{
					// draw the contents of the last shader batch
					Tess_End();
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Beginning Light Interaction: %i -----\n", iaCount));
				}

				// begin a new batch
				Tess_Begin(Tess_StageIteratorStencilLighting, shader, light->shader, qfalse, qfalse, -1);
			}
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			if(drawShadows && !light->l.noShadows)
			{
				GLSL_SetUniform_ModelViewProjectionMatrix(&tr.shadowExtrudeShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
			}

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if(light != oldLight || entity != oldEntity)
		{
			// transform light origin into model space for u_LightOrigin parameter
			if(entity != &tr.worldEntity)
			{
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
			}
			else
			{
				VectorCopy(light->origin, light->transformed);
			}

			if(drawShadows && !light->l.noShadows)
			{
				// set uniform parameter u_LightOrigin for GLSL shader
				GLSL_SetUniform_LightOrigin(&tr.shadowExtrudeShader, light->transformed);
			}

			// build the attenuation matrix using the entity transform
			MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight);

			switch (light->l.rlType)
			{
				case RL_PROJ:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));	// scale
					break;
				}

				case RL_OMNI:
				default:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
					break;
				}
			}
			MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
			MatrixMultiply2(light->attenuationMatrix, modelToLight);
		}

		if(drawShadows)
		{
			// add the triangles for this surface
			rb_surfaceTable[*surface] (surface);
		}
		else
		{
			// add the triangles for this surface
			rb_surfaceTable[*surface] (surface);
		}

	  nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;

	  skipInteraction:
		if(!ia->next)
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if(r_logFile->integer)
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment(va("----- Last Interaction: %i -----\n", iaCount));
			}

			// draw the contents of the last shader batch
			Tess_End();

			if(drawShadows)
			{
				// jump back to first interaction of this light and start lighting
				ia = &backEnd.viewParms.interactions[iaFirst];
				iaCount = iaFirst;
				drawShadows = qfalse;
			}
			else
			{
				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// reset scissor clamping
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// reset depth clamping
	if(qglDepthBoundsEXT)
	{
		qglDisable(GL_DEPTH_BOUNDS_TEST_EXT);
	}

	/*
	   if(qglActiveStencilFaceEXT)
	   {
	   qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	   }
	 */

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_forwardLightingTime = endTime - startTime;
	}
}



/*
=================
RB_RenderInteractionsShadowMapped
=================
*/
static void RB_RenderInteractionsShadowMapped()
{
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	interaction_t  *ia;
	int             iaCount;
	int             iaFirst;
	surfaceType_t  *surface;
	qboolean        depthRange, oldDepthRange;
	qboolean        alphaTest, oldAlphaTest;
	deformType_t	deformType, oldDeformType;
	vec3_t          tmp;
	matrix_t        modelToLight;
	qboolean        drawShadows;
	int             cubeSide;
	int				splitFrustumIndex;
	int             startTime = 0, endTime = 0;
	const matrix_t	bias = {	0.5, 0.0, 0.0, 0.0,
								0.0, 0.5, 0.0, 0.0,
								0.0, 0.0, 0.5, 0.0,
								0.5, 0.5, 0.5, 1.0};

	if(!glConfig.framebufferObjectAvailable || !glConfig.textureFloatAvailable)
	{
		RB_RenderInteractions();
		return;
	}

	GLimp_LogComment("--- RB_RenderInteractionsShadowMapped ---\n");

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	oldDeformType = deformType = DEFORM_TYPE_NONE;
	drawShadows = qtrue;
	cubeSide = 0;
	splitFrustumIndex = 0;

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render interactions
	for(iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;
		alphaTest = shader->alphaTest;

		if(shader->numDeforms)
		{
			deformType = ShaderRequiresCPUDeforms(shader) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		if(light->l.inverseShadows)
		{
			// handle those lights in RB_RenderInteractionsDeferredInverseShadows
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if(iaCount == iaFirst)
		{
			if(drawShadows)
			{
				// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
				GL_BindProgram(NULL);
				GL_State(GLS_DEFAULT);
				//GL_VertexAttribsState(ATTR_POSITION);

				GL_SelectTexture(0);
				GL_Bind(tr.whiteImage);

				if(light->l.noShadows || light->shadowLOD < 0)
				{
					if(r_logFile->integer)
					{
						// don't just call LogComment, or we will get
						// a call to va() every frame!
						GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));
					}

					goto skipInteraction;
				}
				else
				{
					switch (light->l.rlType)
					{
						case RL_OMNI:
						{
							//float           xMin, xMax, yMin, yMax;
							//float           width, height, depth;
							float           zNear, zFar;
							float           fovX, fovY;
							qboolean        flipX, flipY;
							//float          *proj;
							vec3_t          angles;
							matrix_t        rotationMatrix, transformMatrix, viewMatrix;

							if(r_logFile->integer)
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment(va("----- Rendering shadowCube side: %i -----\n", cubeSide));
							}

							R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
							R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + cubeSide,
												 tr.shadowCubeFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							switch (cubeSide)
							{
								case 0:
								{
									// view parameters
									VectorSet(angles, 0, 0, 90);

									// projection parameters
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 1:
								{
									VectorSet(angles, 0, 180, 90);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 2:
								{
									VectorSet(angles, 0, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 3:
								{
									VectorSet(angles, 0, -90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 4:
								{
									VectorSet(angles, -90, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 5:
								{
									VectorSet(angles, 90, 90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								default:
								{
									// shut up compiler
									VectorSet(angles, 0, 0, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}
							}

							// Quake -> OpenGL view matrix from light perspective
							MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
							MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);
							MatrixAffineInverse(transformMatrix, viewMatrix);

							// convert from our coordinate system (looking down X)
							// to OpenGL's coordinate system (looking down -Z)
							MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);

							// OpenGL projection matrix
							fovX = 90;
							fovY = 90;

							zNear = 1.0;
							zFar = light->sphereRadius;

							if(flipX)
							{
								fovX = -fovX;
							}

							if(flipY)
							{
								fovY = -fovY;
							}

							MatrixPerspectiveProjectionFovXYRH(light->projectionMatrix, fovX, fovY, zNear, zFar);

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						case RL_PROJ:
						{
							GLimp_LogComment("--- Rendering projective shadowMap ---\n");

							R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						case RL_DIRECTIONAL:
						{
							int				j;
							vec3_t			angles;
							vec4_t			forward, side, up;
							vec3_t			lightDirection;
							vec3_t			viewOrigin, viewDirection;
							matrix_t        rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
							matrix_t		cropMatrix;
							vec4_t			splitFrustum[6];
							vec3_t			splitFrustumCorners[8];
							vec3_t			splitFrustumBounds[2];
							//vec3_t		splitFrustumViewBounds[2];
							vec3_t			splitFrustumClipBounds[2];
							//float			splitFrustumRadius;
							int				numCasters;
							vec3_t			casterBounds[2];
							vec3_t			receiverBounds[2];
							vec3_t			cropBounds[2];
							vec4_t			point;
							vec4_t			transf;


							GLimp_LogComment("--- Rendering directional shadowMap ---\n");

							R_BindFBO(tr.shadowMapFBO[splitFrustumIndex]);
							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[splitFrustumIndex]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[splitFrustumIndex]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[splitFrustumIndex], shadowMapResolutions[splitFrustumIndex]);
							GL_Scissor(0, 0, shadowMapResolutions[splitFrustumIndex], shadowMapResolutions[splitFrustumIndex]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							#if 1
							VectorCopy(tr.sunDirection, lightDirection);
							#else
							VectorCopy(light->direction, lightDirection);
							#endif

#if 1
							if(r_parallelShadowSplits->integer)
							{
								// original light direction is from surface to light
								VectorInverse(lightDirection);
								VectorNormalize(lightDirection);

								VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
								VectorCopy(backEnd.viewParms.orientation.axis[0], viewDirection);
								VectorNormalize(viewDirection);

#if 1
								// calculate new up dir
								CrossProduct(lightDirection, viewDirection, side);
								VectorNormalize(side);

								CrossProduct(side, lightDirection, up);
								VectorNormalize(up);

								VectorToAngles(lightDirection, angles);
								MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
								AngleVectors(angles, forward, side, up);

								MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, up);
#else
								MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, viewDirection);
#endif

								for(j = 0; j < 6; j++)
								{
									VectorCopy(backEnd.viewParms.frustums[1 + splitFrustumIndex][j].normal, splitFrustum[j]);
									splitFrustum[j][3] = backEnd.viewParms.frustums[1 + splitFrustumIndex][j].dist;
								}

								// calculate split frustum corner points
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[0]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[1]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[2]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[3]);

								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[4]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[5]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[6]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[7]);

								if(r_logFile->integer)
								{
									vec3_t	rayIntersectionNear, rayIntersectionFar;
									float	zNear, zFar;

									// don't just call LogComment, or we will get
									// a call to va() every frame!
									//GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));

									PlaneIntersectRay(viewOrigin, viewDirection, splitFrustum[FRUSTUM_FAR], rayIntersectionFar);
									zFar = Distance(viewOrigin, rayIntersectionFar);

									VectorInverse(viewDirection);

									PlaneIntersectRay(rayIntersectionFar, viewDirection,splitFrustum[FRUSTUM_NEAR], rayIntersectionNear);
									zNear = Distance(viewOrigin, rayIntersectionNear);

									VectorInverse(viewDirection);

									GLimp_LogComment(va("split frustum %i: near = %5.3f, far = %5.3f\n", splitFrustumIndex, zNear, zFar));
									GLimp_LogComment(va("pyramid nearCorners\n"));
									for(j = 0; j < 4; j++)
									{
										GLimp_LogComment(va("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]));
									}

									GLimp_LogComment(va("pyramid farCorners\n"));
									for(j = 4; j < 8; j++)
									{
										GLimp_LogComment(va("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]));
									}
								}

								ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
								for(j = 0; j < 8; j++)
								{
									AddPointToBounds(splitFrustumCorners[j], splitFrustumBounds[0], splitFrustumBounds[1]);
								}


#if 0
								// find the bounding box of the current split in the light's view space
								ClearBounds(splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								numCasters = MergeInteractionBounds(light->viewMatrix, ia, iaCount, splitFrustumViewBounds, qtrue);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;
#if 0
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif

									AddPointToBounds(transf, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								}

								//MatrixScaleTranslateToUnitCube(projectionMatrix, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								MatrixOrthogonalProjectionRH(projectionMatrix, -1, 1, -1, 1, -splitFrustumViewBounds[1][2], -splitFrustumViewBounds[0][2]);

								MatrixMultiply(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

								// find the bounding box of the current split in the light's clip space
								ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;

									MatrixTransform4(viewProjectionMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];

									AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								}
								splitFrustumClipBounds[0][2] = 0;
								splitFrustumClipBounds[1][2] = 1;

								MatrixCrop(cropMatrix, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								//MatrixIdentity(cropMatrix);

								if(r_logFile->integer)
								{
									GLimp_LogComment(va("split frustum light view space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumViewBounds[0][0], splitFrustumViewBounds[0][1], splitFrustumViewBounds[0][2],
														splitFrustumViewBounds[1][0], splitFrustumViewBounds[1][1], splitFrustumViewBounds[1][2]));

									GLimp_LogComment(va("split frustum light clip space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
														splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]));
								}

#else

								// find the bounding box of the current split in the light's view space
								ClearBounds(cropBounds[0], cropBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;
#if 1
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif

									AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
								}

								MatrixOrthogonalProjectionRH(projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);

								MatrixMultiply(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

								numCasters = MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, casterBounds, qtrue);
								MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, receiverBounds, qfalse);

								// find the bounding box of the current split in the light's clip space
								ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;

									MatrixTransform4(viewProjectionMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];

									AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								}


								if(r_logFile->integer)
								{
									GLimp_LogComment(va("shadow casters = %i\n", numCasters));

									GLimp_LogComment(va("split frustum light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
														splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]));

									GLimp_LogComment(va("shadow caster light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														casterBounds[0][0], casterBounds[0][1], casterBounds[0][2],
														casterBounds[1][0], casterBounds[1][1], casterBounds[1][2]));

									GLimp_LogComment(va("light receiver light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														receiverBounds[0][0], receiverBounds[0][1], receiverBounds[0][2],
														receiverBounds[1][0], receiverBounds[1][1], receiverBounds[1][2]));
								}

								// scene-dependent bounding volume
								cropBounds[0][0] = Q_max(Q_max(casterBounds[0][0], receiverBounds[0][0]), splitFrustumClipBounds[0][0]);
								cropBounds[0][1] = Q_max(Q_max(casterBounds[0][1], receiverBounds[0][1]), splitFrustumClipBounds[0][1]);

								cropBounds[1][0] = Q_min(Q_min(casterBounds[1][0], receiverBounds[1][0]), splitFrustumClipBounds[1][0]);
								cropBounds[1][1] = Q_min(Q_min(casterBounds[1][1], receiverBounds[1][1]), splitFrustumClipBounds[1][1]);

								//cropBounds[0][2] = Q_min(casterBounds[0][2], splitFrustumClipBounds[0][2]);
								cropBounds[0][2] = casterBounds[0][2];
								//cropBounds[0][2] = splitFrustumClipBounds[0][2];
								cropBounds[1][2] = Q_min(receiverBounds[1][2], splitFrustumClipBounds[1][2]);
								//cropBounds[1][2] = splitFrustumClipBounds[1][2];

								if(numCasters == 0)
								{
									VectorCopy(splitFrustumClipBounds[0], cropBounds[0]);
									VectorCopy(splitFrustumClipBounds[1], cropBounds[1]);
								}

								MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);
#endif


								MatrixMultiply(cropMatrix, projectionMatrix, light->projectionMatrix);

								GL_LoadProjectionMatrix(light->projectionMatrix);
							}
							else
#endif
							{
								// original light direction is from surface to light
								VectorInverse(lightDirection);

								// Quake -> OpenGL view matrix from light perspective
#if 1
								VectorToAngles(lightDirection, angles);
								MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
								MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin);
								MatrixAffineInverse(transformMatrix, viewMatrix);
								MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
#else
								MatrixLookAtRH(light->viewMatrix, backEnd.viewParms.orientation.origin, lightDirection, backEnd.viewParms.orientation.axis[0]);
#endif

								ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
								//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
								BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

								ClearBounds(cropBounds[0], cropBounds[1]);
								for(j = 0; j < 8; j++)
								{
									point[0] = splitFrustumBounds[j & 1][0];
									point[1] = splitFrustumBounds[(j >> 1) & 1][1];
									point[2] = splitFrustumBounds[(j >> 2) & 1][2];
									point[3] = 1;
#if 1
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif
									AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
								}

								// transform from OpenGL's right handed into D3D's left handed coordinate system
#if 0
								MatrixScaleTranslateToUnitCube(projectionMatrix, cropBounds[0], cropBounds[1]);
								MatrixMultiply(flipZMatrix, projectionMatrix, light->projectionMatrix);
#else
								MatrixOrthogonalProjectionRH(light->projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);
#endif
								GL_LoadProjectionMatrix(light->projectionMatrix);
							}
							break;
						}

						default:
							break;
					}
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Shadow Interaction: %i -----\n", iaCount));
				}
			}
			else
			{
				GLimp_LogComment("--- Rendering lighting ---\n");

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Light Interaction: %i -----\n", iaCount));
				}

				if(r_hdrRendering->integer)
					R_BindFBO(tr.deferredRenderFBO);
				else
					R_BindNullFBO();

				// set the window clipping
				GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
						   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				// restore camera matrices
				GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
				GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

				// reset light view and projection matrices
				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						MatrixAffineInverse(light->transformMatrix, light->viewMatrix);
						MatrixSetupScale(light->projectionMatrix, 1.0 / light->l.radius[0], 1.0 / light->l.radius[1],
										 1.0 / light->l.radius[2]);
						break;
					}

					case RL_DIRECTIONAL:
					{
						// draw split frustum shadow maps
						if(r_showShadowMaps->integer && light->l.rlType == RL_DIRECTIONAL)
						{
							int			frustumIndex;
							float		x, y, w, h;
							matrix_t	ortho;
							vec4_t		quadVerts[4];

							// set 2D virtual screen size
							GL_PushMatrix();
							MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
															backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
															backEnd.viewParms.viewportY,
															backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999);
							GL_LoadProjectionMatrix(ortho);
							GL_LoadModelViewMatrix(matrixIdentity);

							for(frustumIndex = 0; frustumIndex <= r_parallelShadowSplits->integer; frustumIndex++)
							{
								GL_BindProgram(&tr.debugShadowMapShader);
								GL_Cull(CT_TWO_SIDED);
								GL_State(GLS_DEPTHTEST_DISABLE);

								// set uniforms
								GLSL_SetUniform_ModelViewProjectionMatrix(&tr.debugShadowMapShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

								GL_SelectTexture(0);
								GL_Bind(tr.shadowMapFBOImage[frustumIndex]);

								w = 200;
								h = 200;

								x = 205 * frustumIndex;
								y = 70;

								VectorSet4(quadVerts[0], x, y, 0, 1);
								VectorSet4(quadVerts[1], x + w, y, 0, 1);
								VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
								VectorSet4(quadVerts[3], x, y + h, 0, 1);

								Tess_InstantQuad(quadVerts);

								{
									int				j;
									vec4_t			splitFrustum[6];
									vec3_t          farCorners[4];
									vec3_t          nearCorners[4];

									GL_Viewport(x, y, w, h);
									GL_Scissor(x, y, w, h);

									GL_PushMatrix();

									GL_BindProgram(&tr.genericSingleShader);
									GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
									GL_Cull(CT_TWO_SIDED);

									// set uniforms
									GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
									GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
									GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
									if(glConfig.vboVertexSkinningAvailable)
									{
										GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
									}
									GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
									GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

									// bind u_ColorMap
									GL_SelectTexture(0);
									GL_Bind(tr.whiteImage);
									GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

									GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, light->shadowMatrices[frustumIndex]);

									tess.numIndexes = 0;
									tess.numVertexes = 0;

									for(j = 0; j < 6; j++)
									{
										VectorCopy(backEnd.viewParms.frustums[1 + frustumIndex][j].normal, splitFrustum[j]);
										splitFrustum[j][3] = backEnd.viewParms.frustums[1 + frustumIndex][j].dist;
									}

									// calculate split frustum corner points
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[0]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[1]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[2]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[3]);

									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[0]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[1]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[2]);
									PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[3]);

									// draw outer surfaces
									for(j = 0; j < 4; j++)
									{
										VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
										VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
										VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
										VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
										Tess_AddQuadStamp2(quadVerts, colorCyan);
									}

									// draw far cap
									VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
									VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
									VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
									VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
									Tess_AddQuadStamp2(quadVerts, colorBlue);

									// draw near cap
									VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
									VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
									VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
									VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
									Tess_AddQuadStamp2(quadVerts, colorGreen);

									Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
									Tess_DrawElements();

									// draw light volume
									if(light->isStatic && light->frustumVBO && light->frustumIBO)
									{
										GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_CUSTOM_RGB);
										GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_CUSTOM);
										GLSL_SetUniform_Color(&tr.genericSingleShader, colorYellow);

										R_BindVBO(light->frustumVBO);
										R_BindIBO(light->frustumIBO);

										GL_VertexAttribsState(ATTR_POSITION);

										tess.numVertexes = light->frustumVerts;
										tess.numIndexes = light->frustumIndexes;

										Tess_DrawElements();
									}

									tess.numIndexes = 0;
									tess.numVertexes = 0;

									GL_PopMatrix();

									GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
												backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

									GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
											   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
								}
							}

							GL_PopMatrix();
						}
					}

					default:
						break;
				}
			}
		}						// end if(iaCount == iaFirst)

		if(drawShadows)
		{
			if(entity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			{
				goto skipInteraction;
			}

			if(shader->isSky)
			{
				goto skipInteraction;
			}

			if(shader->sort > SS_OPAQUE)
			{
				goto skipInteraction;
			}

			if(shader->noShadows || light->l.noShadows || light->shadowLOD < 0)
			{
				goto skipInteraction;
			}

			/*
			   if(light->l.inverseShadows && (entity == &tr.worldEntity))
			   {
			   // this light only casts shadows by its player and their items
			   goto skipInteraction;
			   }
			 */

			if(ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}

			if(light->l.rlType == RL_OMNI && !(ia->cubeSideBits & (1 << cubeSide)))
			{
				goto skipInteraction;
			}

			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
				{
					if(light == oldLight && entity == oldEntity && (alphaTest ? shader == oldShader : alphaTest == oldAlphaTest) && deformType == oldDeformType)
					{
						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Batching Shadow Interaction: %i -----\n", iaCount));
						}

						// fast path, same as previous
						rb_surfaceTable[*surface] (surface);
						goto nextInteraction;
					}
					else
					{
						if(oldLight)
						{
							// draw the contents of the last shader batch
							Tess_End();
						}

						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Beginning Shadow Interaction: %i -----\n", iaCount));
						}

						// we don't need tangent space calculations here
						Tess_Begin(Tess_StageIteratorShadowFill, shader, light->shader, qtrue, qfalse, -1);
					}
					break;
				}

				default:
					break;
			}
		}
		else
		{
			if(!shader->interactLight)
			{
				goto skipInteraction;
			}

			if(ia->type == IA_SHADOWONLY)
			{
				goto skipInteraction;
			}

			if(light == oldLight && entity == oldEntity && shader == oldShader)
			{
				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Batching Light Interaction: %i -----\n", iaCount));
				}

				// fast path, same as previous
				rb_surfaceTable[*surface] (surface);
				goto nextInteraction;
			}
			else
			{
				if(oldLight)
				{
					// draw the contents of the last shader batch
					Tess_End();
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- Beginning Light Interaction: %i -----\n", iaCount));
				}

				// begin a new batch
				Tess_Begin(Tess_StageIteratorLighting, shader, light->shader, light->l.inverseShadows, qfalse, -1);
			}
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					R_RotateEntityForLight(entity, light, &backEnd.orientation);
				}
				else
				{
					R_RotateEntityForViewParms(entity, &backEnd.viewParms, &backEnd.orientation);
				}

				if(entity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					Com_Memset(&backEnd.orientation, 0, sizeof(backEnd.orientation));

					backEnd.orientation.axis[0][0] = 1;
					backEnd.orientation.axis[1][1] = 1;
					backEnd.orientation.axis[2][2] = 1;
					VectorCopy(light->l.origin, backEnd.orientation.viewOrigin);

					MatrixIdentity(backEnd.orientation.transformMatrix);
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixCopy(backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix);
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if(light != oldLight || entity != oldEntity)
		{
			// transform light origin into model space for u_LightOrigin parameter
			if(entity != &tr.worldEntity)
			{
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
			}
			else
			{
				VectorCopy(light->origin, light->transformed);
			}

			MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight);

			// build the attenuation matrix using the entity transform
			switch (light->l.rlType)
			{
				case RL_PROJ:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));	// scale
					break;
				}

				case RL_OMNI:
				default:
				{
					MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
					MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
					break;
				}
			}
			MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);
			MatrixMultiply2(light->attenuationMatrix, modelToLight);
		}

		if(drawShadows)
		{
			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
				{
					// add the triangles for this surface
					rb_surfaceTable[*surface] (surface);
					break;
				}

				default:
					break;
			}
		}
		else
		{
			// add the triangles for this surface
			rb_surfaceTable[*surface] (surface);
		}

	  nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;
		oldAlphaTest = alphaTest;
		oldDeformType = deformType;

	  skipInteraction:
		if(!ia->next)
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if(r_logFile->integer)
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment(va("----- Last Interaction: %i -----\n", iaCount));
			}

			// draw the contents of the last shader batch
			Tess_End();

			if(drawShadows)
			{
				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						if(cubeSide == 5)
						{
							cubeSide = 0;
							drawShadows = qfalse;
						}
						else
						{
							cubeSide++;
						}

						// jump back to first interaction of this light
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						break;
					}

					case RL_PROJ:
					{
						// jump back to first interaction of this light and start lighting
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						drawShadows = qfalse;
						break;
					}

					case RL_DIRECTIONAL:
					{
						// set shadow matrix including scale + offset
						MatrixCopy(bias, light->shadowMatricesBiased[splitFrustumIndex]);
						MatrixMultiply2(light->shadowMatricesBiased[splitFrustumIndex], light->projectionMatrix);
						MatrixMultiply2(light->shadowMatricesBiased[splitFrustumIndex], light->viewMatrix);

						MatrixMultiply(light->projectionMatrix, light->viewMatrix, light->shadowMatrices[splitFrustumIndex]);

						if(r_parallelShadowSplits->integer)
						{
							if(splitFrustumIndex == r_parallelShadowSplits->integer)
							{
								splitFrustumIndex = 0;
								drawShadows = qfalse;
							}
							else
							{
								splitFrustumIndex++;
							}

							// jump back to first interaction of this light
							ia = &backEnd.viewParms.interactions[iaFirst];
							iaCount = iaFirst;
						}
						else
						{
							// jump back to first interaction of this light and start lighting
							ia = &backEnd.viewParms.interactions[iaFirst];
							iaCount = iaFirst;
							drawShadows = qfalse;
						}
						break;
					}

					default:
						break;
				}
			}
			else
			{
#ifdef VOLUMETRIC_LIGHTING
				// draw the light volume if needed
				if(light->shader->volumetricLight)
				{
					Render_lightVolume(ia);
				}
#endif

				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// reset scissor clamping
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// reset clear color
	GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_forwardLightingTime = endTime - startTime;
	}
}

static void RB_RenderDrawSurfacesIntoGeometricBuffer()
{
	trRefEntity_t  *entity, *oldEntity;
	shader_t       *shader, *oldShader;
	int             lightmapNum, oldLightmapNum;
	qboolean        depthRange, oldDepthRange;
	int             i;
	drawSurf_t     *drawSurf;
	int             startTime = 0, endTime = 0;

	GLimp_LogComment("--- RB_RenderDrawSurfacesIntoGeometricBuffer ---\n");

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldEntity = NULL;
	oldShader = NULL;
	oldLightmapNum = -1;
	oldDepthRange = qfalse;
	depthRange = qfalse;
	backEnd.currentLight = NULL;

	GL_CheckErrors();

	for(i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++)
	{
		// update locals
		entity = drawSurf->entity;
		shader = tr.sortedShaders[drawSurf->shaderNum];
		lightmapNum = drawSurf->lightmapNum;

		// skip all translucent surfaces that don't matter for this pass
		if(shader->sort > SS_OPAQUE)
		{
			break;
		}

		if(entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum)
		{
			// fast path, same as previous sort
			rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
			continue;
		}

		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if(shader != oldShader || (entity != oldEntity && !shader->entityMergable))
		{
			if(oldShader != NULL)
			{
				Tess_End();
			}

			Tess_Begin(Tess_StageIteratorGBuffer, shader, NULL, qfalse, qfalse, lightmapNum);
			oldShader = shader;
			oldLightmapNum = lightmapNum;
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				backEnd.currentEntity = entity;

				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
	}

	// draw the contents of the last shader batch
	if(oldShader != NULL)
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// disable offscreen rendering
	R_BindNullFBO();

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredGBufferTime = endTime - startTime;
	}
}



void RB_RenderInteractionsDeferred()
{
	interaction_t  *ia;
	int             iaCount;
	trRefLight_t   *light, *oldLight = NULL;
	shader_t       *lightShader;
	shaderStage_t  *attenuationXYStage;
	shaderStage_t  *attenuationZStage;
//	int             i;
	int		j;
	vec3_t          viewOrigin;
	vec3_t          lightOrigin;
	vec4_t          lightColor;
	matrix_t        ortho;
	vec4_t          quadVerts[4];
	int             startTime = 0, endTime = 0;

	GLimp_LogComment("--- RB_RenderInteractionsDeferred ---\n");

	if(r_skipLightBuffer->integer)
		return;

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	GL_State(GLS_DEFAULT);

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);


	if(DS_PREPASS_LIGHTING_ENABLED())
	{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.lightRenderFBO);
#else
		R_BindNullFBO();

		// update normal render image
		GL_SelectTexture(0);
		GL_Bind(tr.deferredNormalFBOImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.deferredNormalFBOImage->uploadWidth, tr.deferredNormalFBOImage->uploadHeight);

		// update depth render image
		GL_SelectTexture(1);
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
#endif

		GL_ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		qglClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		R_BindFBO(tr.deferredRenderFBO);
	}

	// update uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);

	// loop trough all light interactions and render the light quad for each last interaction
	for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

	  skipInteraction:
		if(!ia->next)
		{
			if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && ia->occlusionQuerySamples)
			{
				GLimp_LogComment("--- Rendering light volume ---\n");

				GL_BindProgram(&tr.genericSingleShader);
				GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
				GL_Cull(CT_TWO_SIDED);

				// set uniforms
				GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
				GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
				GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
				if(glConfig.vboVertexSkinningAvailable)
				{
					GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
				}
				GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
				GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

				// bind u_ColorMap
				GL_SelectTexture(0);
				GL_Bind(tr.whiteImage);
				GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

				// set light scissor to reduce fillrate
				GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

				// set the reference stencil value
				GL_ClearStencil(128);

				// reset stencil buffer
				qglClear(GL_STENCIL_BUFFER_BIT);

				// use less compare as depthfunc
				// don't write to the color buffer or depth buffer
				// enable stencil testing for this light
				GL_State(GLS_DEPTHFUNC_LESS | GLS_COLORMASK_BITS | GLS_STENCILTEST_ENABLE);

				qglStencilFunc(GL_ALWAYS, 128, 255);
				qglStencilMask(255);

				if(light->isStatic && light->frustumVBO && light->frustumIBO)
				{
					// render in world space
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					R_BindVBO(light->frustumVBO);
					R_BindIBO(light->frustumIBO);

					GL_VertexAttribsState(ATTR_POSITION);

					tess.numVertexes = light->frustumVerts;
					tess.numIndexes = light->frustumIndexes;
				}
				else
				{
					// render in light space
					R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
					GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					tess.numIndexes = 0;
					tess.numVertexes = 0;

					switch (light->l.rlType)
					{
						case RL_OMNI:
						case RL_DIRECTIONAL:
						{
							Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], colorWhite);

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							break;
						}

						case RL_PROJ:
						{
							vec3_t          farCorners[4];
							vec4_t         *frustum = light->localFrustum;

							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

							if(!VectorCompare(light->l.projStart, vec3_origin))
							{
								vec3_t          nearCorners[4];

								// calculate the vertices defining the top area
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

								// draw outer surfaces
								for(j = 0; j < 4; j++)
								{
									VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
									VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
									VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
									VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
									Tess_AddQuadStamp2(quadVerts, colorCyan);
								}

								// draw far cap
								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorRed);

								// draw near cap
								VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
								VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
								VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
								VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorGreen);

							}
							else
							{
								vec3_t	top;

								// no light_start, just use the top vertex (doesn't need to be mirrored)
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

								// draw pyramid
								for(j = 0; j < 4; j++)
								{
									VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(top, tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;
								}

								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorRed);
							}

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							break;
						}

						default:
							break;
					}
				}

				if(r_showShadowVolumes->integer)
				{
					//GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					GL_State(GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					//GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
					//GL_State(GLS_DEPTHMASK_TRUE);
			#if 0
					GL_Cull(CT_FRONT_SIDED);
					//qglColor4f(1.0f, 1.0f, 0.7f, 0.05f);
					qglVertexAttrib4fARB(ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f);
					Tess_DrawElements();
			#endif

			#if 0
					GL_Cull(CT_BACK_SIDED);
					qglVertexAttrib4fARB(ATTR_INDEX_COLOR, 0.0f, 1.0f, 0.0f, 0.05f);
					Tess_DrawElements();
			#endif

			#if 1
					GL_State(GLS_DEPTHFUNC_LESS | GLS_POLYMODE_LINE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					GL_Cull(CT_TWO_SIDED);
					//qglVertexAttrib4fARB(ATTR_INDEX_COLOR, 0.0f, 0.0f, 1.0f, 0.05f);
					Tess_DrawElements();
			#endif
				}
				else
				{
					if(qglStencilFuncSeparateATI && qglStencilOpSeparateATI && glConfig.stencilWrapAvailable)
					{
						GL_Cull(CT_TWO_SIDED);

						qglStencilFuncSeparateATI(GL_ALWAYS, GL_ALWAYS, 0, (GLuint) ~ 0);

						qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

						Tess_DrawElements();
					}
					else if(qglActiveStencilFaceEXT)
					{
						// render both sides at once
						GL_Cull(CT_TWO_SIDED);

						qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

						qglActiveStencilFaceEXT(GL_BACK);
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
						}

						qglActiveStencilFaceEXT(GL_FRONT);
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
						}

						Tess_DrawElements();

						qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
					}
					else
					{
						// draw only the front faces of the shadow volume
						GL_Cull(CT_FRONT_SIDED);

						// increment the stencil value on zfail
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
						}

						Tess_DrawElements();

						// draw only the back faces of the shadow volume
						GL_Cull(CT_BACK_SIDED);

						// decrement the stencil value on zfail
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
						}

						Tess_DrawElements();
					}
				}

				GL_CheckErrors();

				GLimp_LogComment("--- Rendering lighting ---\n");

				qglStencilFunc(GL_NOTEQUAL, 128, 255);


				if(qglActiveStencilFaceEXT)
				{
					qglActiveStencilFaceEXT(GL_BACK);
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

					qglActiveStencilFaceEXT(GL_FRONT);
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}
				else
				{
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}

				// build world to light space matrix
				switch (light->l.rlType)
				{
					case RL_OMNI:
					case RL_DIRECTIONAL:
					{
						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);
						break;
					}

					case RL_PROJ:
					{
						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, Q_min(light->falloffLength, 1.0));	// scale
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);
						break;
					}

					default:
						break;
				}

				// set 2D virtual screen size
				GL_PushMatrix();
				MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
												backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
												backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
												-99999, 99999);
				GL_LoadProjectionMatrix(ortho);
				GL_LoadModelViewMatrix(matrixIdentity);


				// last interaction of current light
				lightShader = light->shader;
				attenuationZStage = lightShader->stages[0];

				for(j = 1; j < MAX_SHADER_STAGES; j++)
				{
					attenuationXYStage = lightShader->stages[j];

					if(!attenuationXYStage)
					{
						break;
					}

					if(attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
					{
						continue;
					}

					if(!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
					{
						continue;
					}

					Tess_ComputeColor(attenuationXYStage);
					R_ComputeFinalAttenuation(attenuationXYStage, light);

					if(light->l.rlType == RL_OMNI)
					{
						// enable shader, set arrays
						GL_BindProgram(&tr.deferredLightingShader_DBS_omni);

						// set OpenGL state for additive lighting
						GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);

						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);

						GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_omni, viewOrigin);
						GLSL_SetUniform_LightOrigin(&tr.deferredLightingShader_DBS_omni, lightOrigin);
						GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_omni, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_omni, light->sphereRadius);
						GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_omni, light->l.scale);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_omni, light->attenuationMatrix2);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_omni, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_omni, backEnd.viewParms.unprojectionMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_omni, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_omni, plane);
						}

						if(DS_STANDARD_ENABLED())
						{
							// bind u_DiffuseMap
							GL_SelectTexture(0);
							GL_Bind(tr.deferredDiffuseFBOImage);
						}

						// bind u_NormalMap
						GL_SelectTexture(1);
						GL_Bind(tr.deferredNormalFBOImage);

						if(DS_STANDARD_ENABLED() && r_normalMapping->integer)
						{
							// bind u_SpecularMap
							GL_SelectTexture(2);
							GL_Bind(tr.deferredSpecularFBOImage);
						}

						// bind u_DepthMap
						GL_SelectTexture(3);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(4);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(5);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

#if 0
						// draw lighting with a fullscreen quad
						Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
#endif
					}
					else if(light->l.rlType == RL_PROJ)
					{
						// enable shader, set arrays
						GL_BindProgram(&tr.deferredLightingShader_DBS_proj);

						// set OpenGL state for additive lighting
						GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);

						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);

						GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_proj, viewOrigin);
						GLSL_SetUniform_LightOrigin(&tr.deferredLightingShader_DBS_proj, lightOrigin);
						GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_proj, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_proj, light->sphereRadius);
						GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_proj, light->l.scale);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_proj, light->attenuationMatrix2);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_proj, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_proj, backEnd.viewParms.unprojectionMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_proj, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_proj, plane);
						}

						// bind u_NormalMap
						GL_SelectTexture(1);
						GL_Bind(tr.deferredNormalFBOImage);

						// bind u_DepthMap
						GL_SelectTexture(3);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(4);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(5);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

#if 0
						// draw lighting with a fullscreen quad
						Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
#endif
					}
					else if(light->l.rlType == RL_DIRECTIONAL)
					{
						// enable shader, set arrays
						GL_BindProgram(&tr.deferredLightingShader_DBS_directional);

						// set OpenGL state for additive lighting
						GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);

						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);

						GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_directional, viewOrigin);

						//if(VectorLength(light->))
						GLSL_SetUniform_LightDir(&tr.deferredLightingShader_DBS_directional, tr.sunDirection);


						GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_directional, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_directional, light->sphereRadius);
						GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_directional, light->l.scale);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_directional, light->attenuationMatrix2);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_directional, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.unprojectionMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_directional, plane);
						}

						if(DS_STANDARD_ENABLED())
						{
							// bind u_DiffuseMap
							GL_SelectTexture(0);
							GL_Bind(tr.deferredDiffuseFBOImage);
						}

						// bind u_NormalMap
						GL_SelectTexture(1);
						GL_Bind(tr.deferredNormalFBOImage);

						if(DS_STANDARD_ENABLED() && r_normalMapping->integer)
						{
							// bind u_SpecularMap
							GL_SelectTexture(2);
							GL_Bind(tr.deferredSpecularFBOImage);
						}

						// bind u_DepthMap
						GL_SelectTexture(3);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(4);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(5);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

#if 0
						// draw lighting with a fullscreen quad
						Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
#endif
					}
				}

				GL_PopMatrix();
			}

			if(iaCount < (backEnd.viewParms.numInteractions - 1))
			{
				// jump to next interaction and continue
				ia++;
				iaCount++;
			}
			else
			{
				// increase last time to leave for loop
				iaCount++;
			}
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}

		oldLight = light;
	}

	// go back to the world modelview matrix
	backEnd.orientation = backEnd.viewParms.world;
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

	// reset scissor
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredLightingTime = endTime - startTime;
	}
}

static void RB_RenderInteractionsDeferredShadowMapped()
{
	interaction_t  *ia;
	int             iaCount;
	int             iaFirst;
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	surfaceType_t  *surface;
	qboolean        depthRange, oldDepthRange;
	qboolean        alphaTest, oldAlphaTest;
	deformType_t	deformType, oldDeformType;
	qboolean        drawShadows;
	int             cubeSide;

	int				splitFrustumIndex;
	const matrix_t	bias = {	0.5, 0.0, 0.0, 0.0,
								0.0, 0.5, 0.0, 0.0,
								0.0, 0.0, 0.5, 0.0,
								0.5, 0.5, 0.5, 1.0};

	shader_t       *lightShader;
	shaderStage_t  *attenuationXYStage;
	shaderStage_t  *attenuationZStage;
	int             i, j;
	vec3_t          viewOrigin;
	vec3_t          lightOrigin;
	vec3_t			lightDirection;
	vec4_t          lightColor;
	qboolean        shadowCompare;
	matrix_t        ortho;
	vec4_t          quadVerts[4];
	int             startTime = 0, endTime = 0;

	GLimp_LogComment("--- RB_RenderInteractionsDeferredShadowMapped ---\n");

	if(r_skipLightBuffer->integer)
		return;

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	oldDeformType = deformType = DEFORM_TYPE_NONE;
	drawShadows = qtrue;
	cubeSide = 0;
	splitFrustumIndex = 0;

	GL_State(GLS_DEFAULT);

	// set the window clipping
	GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);


	if(DS_PREPASS_LIGHTING_ENABLED())
	{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.lightRenderFBO);
#else
		R_BindNullFBO();

		// update normal render image
		GL_SelectTexture(0);
		GL_Bind(tr.deferredNormalFBOImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.deferredNormalFBOImage->uploadWidth, tr.deferredNormalFBOImage->uploadHeight);

		// update depth render image
		GL_SelectTexture(1);
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
#endif

		GL_ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		qglClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		R_BindFBO(tr.deferredRenderFBO);
	}

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render interactions
	for(iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;
		alphaTest = shader->alphaTest;

		if(shader->numDeforms)
		{
			deformType = ShaderRequiresCPUDeforms(shader) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if(iaCount == iaFirst)
		{
			if(drawShadows)
			{
				// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
				GL_BindProgram(NULL);
				GL_State(GLS_DEFAULT);
				//GL_VertexAttribsState(ATTR_POSITION);

				GL_SelectTexture(0);
				GL_Bind(tr.whiteImage);

				if(light->l.noShadows || light->shadowLOD < 0)
				{
					if(r_logFile->integer)
					{
						// don't just call LogComment, or we will get
						// a call to va() every frame!
						GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));
					}

					goto skipInteraction;
				}
				else
				{
					switch (light->l.rlType)
					{
						case RL_OMNI:
						{
							float           zNear, zFar;
							float           fovX, fovY;
							qboolean        flipX, flipY;
							//float        *proj;
							vec3_t          angles;
							matrix_t        rotationMatrix, transformMatrix, viewMatrix;

							if(r_logFile->integer)
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment(va("----- Rendering shadowCube side: %i -----\n", cubeSide));
							}

							R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
							R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + cubeSide,
												 tr.shadowCubeFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							switch (cubeSide)
							{
								case 0:
								{
									// view parameters
									VectorSet(angles, 0, 0, 90);

									// projection parameters
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 1:
								{
									VectorSet(angles, 0, 180, 90);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 2:
								{
									VectorSet(angles, 0, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 3:
								{
									VectorSet(angles, 0, -90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 4:
								{
									VectorSet(angles, -90, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 5:
								{
									VectorSet(angles, 90, 90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								default:
								{
									// shut up compiler
									VectorSet(angles, 0, 0, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}
							}

							// Quake -> OpenGL view matrix from light perspective
							MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
							MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);
							MatrixAffineInverse(transformMatrix, viewMatrix);

							// convert from our coordinate system (looking down X)
							// to OpenGL's coordinate system (looking down -Z)
							MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);

							// OpenGL projection matrix
							fovX = 90;
							fovY = 90;

							zNear = 1.0;
							zFar = light->sphereRadius;

							if(flipX)
							{
								fovX = -fovX;
							}

							if(flipY)
							{
								fovY = -fovY;
							}

							MatrixPerspectiveProjectionFovXYRH(light->projectionMatrix, fovX, fovY, zNear, zFar);

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						case RL_PROJ:
						{
							GLimp_LogComment("--- Rendering projective shadowMap ---\n");

							R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						case RL_DIRECTIONAL:
						{
							vec3_t			angles;
							vec4_t			forward, side, up;
							vec3_t			viewOrigin, viewDirection;
							matrix_t        rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
							matrix_t		cropMatrix;
							vec4_t			splitFrustum[6];
							vec3_t			splitFrustumCorners[8];
							vec3_t			splitFrustumBounds[2];
							//vec3_t		splitFrustumViewBounds[2];
							vec3_t			splitFrustumClipBounds[2];
							//float			splitFrustumRadius;
							int				numCasters;
							vec3_t			casterBounds[2];
							vec3_t			receiverBounds[2];
							vec3_t			cropBounds[2];
							vec4_t			point;
							vec4_t			transf;

							GLimp_LogComment("--- Rendering directional shadowMap ---\n");

							R_BindFBO(tr.shadowMapFBO[splitFrustumIndex]);
							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[splitFrustumIndex]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[splitFrustumIndex]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[splitFrustumIndex], shadowMapResolutions[splitFrustumIndex]);
							GL_Scissor(0, 0, shadowMapResolutions[splitFrustumIndex], shadowMapResolutions[splitFrustumIndex]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#if 1
							VectorCopy(tr.sunDirection, lightDirection);
#else
							VectorCopy(light->direction, lightDirection);
#endif


#if 0
							if(r_lightSpacePerspectiveWarping->integer)
							{
								vec3_t			viewOrigin, viewDirection;
								vec4_t			forward, side, up;
								matrix_t		lispMatrix;
								matrix_t		postMatrix;
								matrix_t		projectionCenter;

								const matrix_t switchToArticle = {
									1, 0, 0, 0,
									0, 0, 1, 0,
									0, -1, 0, 0,
									0, 0, 0, 1
								};

								const matrix_t switchToGL = {
									1, 0, 0, 0,
									0, 0, -1, 0,
									0, 1, 0, 0,
									0, 0, 0, 1
								};

								// original light direction is from surface to light
								VectorInverse(lightDirection);
								VectorNormalize(lightDirection);

								VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
								VectorCopy(backEnd.viewParms.orientation.axis[0], viewDirection);
								VectorNormalize(viewDirection);

								// calculate new up dir
								CrossProduct(lightDirection, viewDirection, side);
								VectorNormalize(side);

								CrossProduct(side, lightDirection, up);
								VectorNormalize(up);

#if 0
								VectorToAngles(lightDirection, angles);
								MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
								AngleVectors(angles, forward, side, up);
#endif

								MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, up);

#if 0
								ri.Printf(PRINT_ALL, "light = (%5.3f, %5.3f, %5.3f)\n", lightDirection[0], lightDirection[1], lightDirection[2]);
								ri.Printf(PRINT_ALL, "side = (%5.3f, %5.3f, %5.3f)\n", side[0], side[1], side[2]);
								ri.Printf(PRINT_ALL, "up = (%5.3f, %5.3f, %5.3f)\n", up[0], up[1], up[2]);
#endif


#if 0
								for(j = 0; j < 6; j++)
								{
									VectorCopy(backEnd.viewParms.frustums[splitFrustumIndex][j].normal, splitFrustum[j]);
									splitFrustum[j][3] = backEnd.viewParms.frustums[splitFrustumIndex][j].dist;
								}

								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[0]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[1]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[2]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[3]);

								#if 0
								ri.Printf(PRINT_ALL, "split frustum %i\n", splitFrustumIndex);
								ri.Printf(PRINT_ALL, "pyramid nearCorners\n");
								for(j = 0; j < 4; j++)
								{
									ri.Printf(PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]);
								}
								#endif

								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[4]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[5]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[6]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[7]);

								#if 0
								ri.Printf(PRINT_ALL, "pyramid farCorners\n");
								for(j = 4; j < 8; j++)
								{
									ri.Printf(PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]);
								}
								#endif
#endif

								ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
#if 0
								for(i = 0; i < 8; i++)
								{
									AddPointToBounds(splitFrustumCorners[i], splitFrustumBounds[0], splitFrustumBounds[1]);
								}
#endif
								//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
								BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

								ClearBounds(cropBounds[0], cropBounds[1]);
								for(j = 0; j < 8; j++)
								{
									point[0] = splitFrustumBounds[j & 1][0];
									point[1] = splitFrustumBounds[(j >> 1) & 1][1];
									point[2] = splitFrustumBounds[(j >> 2) & 1][2];
									point[3] = 1;

#if 1
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif
									AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
								}


#if 0
								MatrixOrthogonalProjection(projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], cropBounds[0][2], cropBounds[1][2]);
								MatrixMultiply(projectionMatrix, light->viewMatrix, viewProjectionMatrix);
								MatrixMultiply(viewProjectionMatrix, backEnd.viewParms.world.viewMatrix, postMatrix);

								VectorSet(viewOrigin, 0, 0, 0);
								MatrixTransformPoint2(viewOrigin);

								VectorSet(viewDirection, 0, 0, -1);
								MatrixTransformPoint2(viewDirection);

								VectorSet(up, 0, 1, 0);
								MatrixTransformPoint2(viewDirection);
#endif

#if 0
								ri.Printf(PRINT_ALL, "light space crop bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										cropBounds[0][0], cropBounds[0][1], cropBounds[0][2],
										cropBounds[1][0], cropBounds[1][1], cropBounds[1][2]);
#endif

#if 0
								ri.Printf(PRINT_ALL, "cropMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
												   cropMatrix[0], cropMatrix[4], cropMatrix[8], cropMatrix[12],
												   cropMatrix[1], cropMatrix[5], cropMatrix[9], cropMatrix[13],
												   cropMatrix[2], cropMatrix[6], cropMatrix[10], cropMatrix[14],
												   cropMatrix[3], cropMatrix[7], cropMatrix[11], cropMatrix[15]);
#endif

								{
									float gamma;
									float cosGamma;
									float sinGamma;
									float zNear, zFar;
									float depth;
									float n, f;
									vec3_t viewOriginLS, Cstart_lp, C;

									// use the formulas of the paper to get n (and f)
#if 0
									cosGamma = DotProduct(viewDirection, lightDirection);
									sinGamma = sqrt(1.0f - cosGamma * cosGamma);
#else
									gamma = AngleBetweenVectors(viewDirection, lightDirection);
									sinGamma = sin(DEG2RAD(gamma));
#endif

									depth = fabs(cropBounds[1][1] - cropBounds[0][1]); //perspective transform depth //light space y extents
									//depth = fabs(cropBounds[0][2]) + fabs(cropBounds[1][2]);

#if 1
									zNear = backEnd.viewParms.zNear / sinGamma;
									zFar = zNear + depth * sinGamma;
									n = (zNear + sqrt(zFar * zNear)) / sinGamma;
#elif 0
									zNear = backEnd.viewParms.zNear;
									zFar = backEnd.viewParms.zFar;

									n = (zNear + sqrt(zFar * zNear)) / sinGamma;
#else
									zNear = backEnd.viewParms.zNear;
									zFar = zNear + depth * sinGamma;
									n = (zNear + sqrt(zFar * zNear)) / sinGamma;
#endif
									f = n + depth;

									ri.Printf(PRINT_ALL, "gamma = %5.3f, sin(gamma) = %5.3f, n = %5.3f, f = %5.3f\n", gamma, sinGamma, n, f);

									// new observer point n-1 behind eye position:  pos = eyePos-up*(n-nearDist)
#if 1
									VectorMA(viewOrigin, -(n - zNear), up, C);
									//VectorMA(C, depth * 0.5f, lightDirection, C);
#else
									// get the coordinates of the near camera point in light space
									MatrixTransformPoint(light->viewMatrix, viewOrigin, viewOriginLS);

									// c start has the x and y coordinate of e, the z coord of B.min()
									VectorSet(Cstart_lp, viewOriginLS[0], viewOriginLS[1], depth * 0.5f);

									// calc C the projection center
									// new projection center C, n behind the near plane of P
									VectorMA(Cstart_lp, -n, axisDefault[1], C);
									MatrixAffineInverse(light->viewMatrix, transformMatrix);
									MatrixTransformPoint2(transformMatrix, C);
#endif

#if 1
									MatrixLookAtRH(light->viewMatrix, C, lightDirection, up);
#else
									VectorInverse(up);
									MatrixLookAtRH(light->viewMatrix, C, up, lightDirection);
									VectorInverse(up);

									//MatrixLookAtRH(light->viewMatrix, viewOrigin, viewDirection, backEnd.viewParms.orientation.axis[2]);
#endif

#if 0
									if(n >= FLT_MAX)
									{
										// if n is infinite than we should do uniform shadow mapping
										MatrixIdentity(lispMatrix);
									}
									else
#endif
									{
										// one possibility for a simple perspective transformation matrix
										// with the two parameters n(near) and f(far) in y direction
										float			a, b;
										float          *m = lispMatrix;

										a = (f + n) / (f - n);
										b = -2 * f * n / (f - n);

										//a = f / (n - f);
										//b = (n * f) / (n - f);

										m[0] = 1;	m[4] = 0;	m[8] = 0;	m[12] = 0;
										m[1] = 0;	m[5] = a;	m[9] = 0;	m[13] = b;
										m[2] = 0;	m[6] = 0;	m[10] = 1;	m[14] = 0;
										m[3] = 0;	m[7] = 1;	m[11] = 0;	m[15] = 0;

										//MatrixPerspectiveProjectionRH(lispMatrix, -1, 1, n, f, -1, 1);
										//MatrixPerspectiveProjection(lispMatrix, -1, 1, -1, 1, n, f);
										//MatrixInverse(lispMatrix);

										//MatrixPerspectiveProjectionLH(lispMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], cropBounds[0][2], cropBounds[1][2]);
										//MatrixPerspectiveProjectionRH(lispMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -f, -n);

#if 0
										ri.Printf(PRINT_ALL, "lispMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
													   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
													   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
													   "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
													   m[0], m[4], m[8], m[12],
													   m[1], m[5], m[9], m[13],
													   m[2], m[6], m[10], m[14],
													   m[3], m[7], m[11], m[15]);
#endif
									}

									//MatrixIdentity(lispMatrix);

									// temporal arrangement for the transformation of the points to post-perspective space
#if 0
									MatrixCopy(flipZMatrix, viewProjectionMatrix);
									//MatrixMultiply2(viewProjectionMatrix, switchToGL);
									MatrixMultiply2(viewProjectionMatrix, lispMatrix);
									//MatrixMultiply2(viewProjectionMatrix, switchToGL);
										//MatrixMultiplyScale(viewProjectionMatrix, 1, 1, -1);
									//MatrixMultiply(flipZMatrix, lispMatrix, viewProjectionMatrix);
									//MatrixMultiply(lispMatrix, light->viewMatrix, viewProjectionMatrix);
									//MatrixMultiply2(viewProjectionMatrix, cropMatrix);
									MatrixMultiply2(viewProjectionMatrix, light->viewMatrix);
									//MatrixMultiply2(viewProjectionMatrix, projectionCenter);
									//MatrixMultiply2(viewProjectionMatrix, transformMatrix);
#else
									MatrixMultiply(lispMatrix, light->viewMatrix, viewProjectionMatrix);
									//MatrixMultiply(flipZMatrix, lispMatrix, viewProjectionMatrix);
									//MatrixMultiply2(viewProjectionMatrix, light->viewMatrix);
#endif
									//MatrixMultiply(lispMatrix, light->viewMatrix, viewProjectionMatrix);
									//MatrixMultiply(light->viewMatrix, lispMatrix, viewProjectionMatrix);

									//transform the light volume points from world into the distorted light space
									//transformVecPoint(&Bcopy,lightProjection);

									//calculate the cubic hull (an AABB)
									//of the light space extents of the intersection body B
									//and save the two extreme points min and max
									//calcCubicHull(min,max,Bcopy.points,Bcopy.size);

#if 0

									VectorSet(forward, 1, 0, 0);
									VectorSet(side, 0, 1, 0);
									VectorSet(up, 0, 0, 1);

									MatrixTransformNormal2(viewProjectionMatrix, forward);
									MatrixTransformNormal2(viewProjectionMatrix, side);
									MatrixTransformNormal2(viewProjectionMatrix, up);

									ri.Printf(PRINT_ALL, "forward = (%5.3f, %5.3f, %5.3f)\n", forward[0], forward[1], forward[2]);
									ri.Printf(PRINT_ALL, "side = (%5.3f, %5.3f, %5.3f)\n", side[0], side[1], side[2]);
									ri.Printf(PRINT_ALL, "up = (%5.3f, %5.3f, %5.3f)\n", up[0], up[1], up[2]);
#endif

#if 1
									ClearBounds(cropBounds[0], cropBounds[1]);
									for(j = 0; j < 8; j++)
									{
										point[0] = splitFrustumBounds[j & 1][0];
										point[1] = splitFrustumBounds[(j >> 1) & 1][1];
										point[2] = splitFrustumBounds[(j >> 2) & 1][2];
										point[3] = 1;

										MatrixTransform4(viewProjectionMatrix, point, transf);
										transf[0] /= transf[3];
										transf[1] /= transf[3];
										transf[2] /= transf[3];

										AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
									}

									MatrixScaleTranslateToUnitCube(cropMatrix, cropBounds[0], cropBounds[1]);
									//MatrixOrthogonalProjection(cropMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], cropBounds[0][2], cropBounds[1][2]);
#endif
								}

#if 0
								ri.Printf(PRINT_ALL, "light space post crop bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
																	cropBounds[0][0], cropBounds[0][1], cropBounds[0][2],
																	cropBounds[1][0], cropBounds[1][1], cropBounds[1][2]);
#endif

								//
								//MatrixInverse(cropMatrix);

#if 0
								ri.Printf(PRINT_ALL, "cropMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
												   "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
												   cropMatrix[0], cropMatrix[4], cropMatrix[8], cropMatrix[12],
												   cropMatrix[1], cropMatrix[5], cropMatrix[9], cropMatrix[13],
												   cropMatrix[2], cropMatrix[6], cropMatrix[10], cropMatrix[14],
												   cropMatrix[3], cropMatrix[7], cropMatrix[11], cropMatrix[15]);
#endif

								//MatrixOrthogonalProjectionLH(cropMatrix, -1024, 1024, -1024, 1024, cropBounds[0][2], cropBounds[1][2]);

								//MatrixIdentity(light->projectionMatrix);
								MatrixCopy(flipZMatrix, light->projectionMatrix);

								//MatrixMultiply2(light->projectionMatrix, switchToArticle);
								MatrixMultiply2(light->projectionMatrix, cropMatrix);
								MatrixMultiply2(light->projectionMatrix, lispMatrix);

								GL_LoadProjectionMatrix(light->projectionMatrix);
							}
							else
#endif
							if(r_parallelShadowSplits->integer)
							{
								// original light direction is from surface to light
								VectorInverse(lightDirection);
								VectorNormalize(lightDirection);

								VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);
								VectorCopy(backEnd.viewParms.orientation.axis[0], viewDirection);
								VectorNormalize(viewDirection);

#if 1
								// calculate new up dir
								CrossProduct(lightDirection, viewDirection, side);
								VectorNormalize(side);

								CrossProduct(side, lightDirection, up);
								VectorNormalize(up);

								VectorToAngles(lightDirection, angles);
								MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
								AngleVectors(angles, forward, side, up);

								MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, up);
#else
								MatrixLookAtRH(light->viewMatrix, viewOrigin, lightDirection, viewDirection);
#endif

								for(j = 0; j < 6; j++)
								{
									VectorCopy(backEnd.viewParms.frustums[1 + splitFrustumIndex][j].normal, splitFrustum[j]);
									splitFrustum[j][3] = backEnd.viewParms.frustums[1 + splitFrustumIndex][j].dist;
								}

								// calculate split frustum corner points
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[0]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[1]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[2]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], splitFrustumCorners[3]);

								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[4]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[5]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[6]);
								PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], splitFrustumCorners[7]);

								if(r_logFile->integer)
								{
									vec3_t	rayIntersectionNear, rayIntersectionFar;
									float	zNear, zFar;

									// don't just call LogComment, or we will get
									// a call to va() every frame!
									//GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));

									PlaneIntersectRay(viewOrigin, viewDirection, splitFrustum[FRUSTUM_FAR], rayIntersectionFar);
									zFar = Distance(viewOrigin, rayIntersectionFar);

									VectorInverse(viewDirection);

									PlaneIntersectRay(rayIntersectionFar, viewDirection,splitFrustum[FRUSTUM_NEAR], rayIntersectionNear);
									zNear = Distance(viewOrigin, rayIntersectionNear);

									VectorInverse(viewDirection);

									GLimp_LogComment(va("split frustum %i: near = %5.3f, far = %5.3f\n", splitFrustumIndex, zNear, zFar));
									GLimp_LogComment(va("pyramid nearCorners\n"));
									for(j = 0; j < 4; j++)
									{
										GLimp_LogComment(va("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]));
									}

									GLimp_LogComment(va("pyramid farCorners\n"));
									for(j = 4; j < 8; j++)
									{
										GLimp_LogComment(va("(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[j][0], splitFrustumCorners[j][1], splitFrustumCorners[j][2]));
									}
								}

								ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
								for(i = 0; i < 8; i++)
								{
									AddPointToBounds(splitFrustumCorners[i], splitFrustumBounds[0], splitFrustumBounds[1]);
								}


#if 0
								// find the bounding box of the current split in the light's view space
								ClearBounds(splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								numCasters = MergeInteractionBounds(light->viewMatrix, ia, iaCount, splitFrustumViewBounds, qtrue);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;
#if 0
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif

									AddPointToBounds(transf, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								}

								//MatrixScaleTranslateToUnitCube(projectionMatrix, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
								MatrixOrthogonalProjectionRH(projectionMatrix, -1, 1, -1, 1, -splitFrustumViewBounds[1][2], -splitFrustumViewBounds[0][2]);

								MatrixMultiply(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

								// find the bounding box of the current split in the light's clip space
								ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;

									MatrixTransform4(viewProjectionMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];

									AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								}
								splitFrustumClipBounds[0][2] = 0;
								splitFrustumClipBounds[1][2] = 1;

								MatrixCrop(cropMatrix, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								//MatrixIdentity(cropMatrix);

								if(r_logFile->integer)
								{
									GLimp_LogComment(va("split frustum light view space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumViewBounds[0][0], splitFrustumViewBounds[0][1], splitFrustumViewBounds[0][2],
														splitFrustumViewBounds[1][0], splitFrustumViewBounds[1][1], splitFrustumViewBounds[1][2]));

									GLimp_LogComment(va("split frustum light clip space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
														splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]));
								}

#else

								// find the bounding box of the current split in the light's view space
								ClearBounds(cropBounds[0], cropBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;
#if 1
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif

									AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
								}

								MatrixOrthogonalProjectionRH(projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);

								MatrixMultiply(projectionMatrix, light->viewMatrix, viewProjectionMatrix);

								numCasters = MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, casterBounds, qtrue);
								MergeInteractionBounds(viewProjectionMatrix, ia, iaCount, receiverBounds, qfalse);

								// find the bounding box of the current split in the light's clip space
								ClearBounds(splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								for(j = 0; j < 8; j++)
								{
									VectorCopy(splitFrustumCorners[j], point);
									point[3] = 1;

									MatrixTransform4(viewProjectionMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];

									AddPointToBounds(transf, splitFrustumClipBounds[0], splitFrustumClipBounds[1]);
								}


								if(r_logFile->integer)
								{
									GLimp_LogComment(va("shadow casters = %i\n", numCasters));

									GLimp_LogComment(va("split frustum light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														splitFrustumClipBounds[0][0], splitFrustumClipBounds[0][1], splitFrustumClipBounds[0][2],
														splitFrustumClipBounds[1][0], splitFrustumClipBounds[1][1], splitFrustumClipBounds[1][2]));

									GLimp_LogComment(va("shadow caster light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														casterBounds[0][0], casterBounds[0][1], casterBounds[0][2],
														casterBounds[1][0], casterBounds[1][1], casterBounds[1][2]));

									GLimp_LogComment(va("light receiver light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
														receiverBounds[0][0], receiverBounds[0][1], receiverBounds[0][2],
														receiverBounds[1][0], receiverBounds[1][1], receiverBounds[1][2]));
								}

								// scene-dependent bounding volume
								cropBounds[0][0] = Q_max(Q_max(casterBounds[0][0], receiverBounds[0][0]), splitFrustumClipBounds[0][0]);
								cropBounds[0][1] = Q_max(Q_max(casterBounds[0][1], receiverBounds[0][1]), splitFrustumClipBounds[0][1]);

								cropBounds[1][0] = Q_min(Q_min(casterBounds[1][0], receiverBounds[1][0]), splitFrustumClipBounds[1][0]);
								cropBounds[1][1] = Q_min(Q_min(casterBounds[1][1], receiverBounds[1][1]), splitFrustumClipBounds[1][1]);

								//cropBounds[0][2] = Q_min(casterBounds[0][2], splitFrustumClipBounds[0][2]);
								cropBounds[0][2] = casterBounds[0][2];
								//cropBounds[0][2] = splitFrustumClipBounds[0][2];
								cropBounds[1][2] = Q_min(receiverBounds[1][2], splitFrustumClipBounds[1][2]);
								//cropBounds[1][2] = splitFrustumClipBounds[1][2];

								if(numCasters == 0)
								{
									VectorCopy(splitFrustumClipBounds[0], cropBounds[0]);
									VectorCopy(splitFrustumClipBounds[1], cropBounds[1]);
								}

								MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);
#endif


								MatrixMultiply(cropMatrix, projectionMatrix, light->projectionMatrix);

								GL_LoadProjectionMatrix(light->projectionMatrix);
							}
							else
							{
								// original light direction is from surface to light
								VectorInverse(lightDirection);

								// Quake -> OpenGL view matrix from light perspective
#if 1
								VectorToAngles(lightDirection, angles);
								MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
								MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin);
								MatrixAffineInverse(transformMatrix, viewMatrix);
								MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
#else
								MatrixLookAtRH(light->viewMatrix, backEnd.viewParms.orientation.origin, lightDirection, backEnd.viewParms.orientation.axis[0]);
#endif

								ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
								//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
								BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

								ClearBounds(cropBounds[0], cropBounds[1]);
								for(j = 0; j < 8; j++)
								{
									point[0] = splitFrustumBounds[j & 1][0];
									point[1] = splitFrustumBounds[(j >> 1) & 1][1];
									point[2] = splitFrustumBounds[(j >> 2) & 1][2];
									point[3] = 1;
#if 1
									MatrixTransform4(light->viewMatrix, point, transf);
									transf[0] /= transf[3];
									transf[1] /= transf[3];
									transf[2] /= transf[3];
#else
									MatrixTransformPoint(light->viewMatrix, point, transf);
#endif
									AddPointToBounds(transf, cropBounds[0], cropBounds[1]);
								}

								// transform from OpenGL's right handed into D3D's left handed coordinate system
#if 0
								MatrixScaleTranslateToUnitCube(projectionMatrix, cropBounds[0], cropBounds[1]);
								MatrixMultiply(flipZMatrix, projectionMatrix, light->projectionMatrix);
#else
								MatrixOrthogonalProjectionRH(light->projectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2]);
#endif
								GL_LoadProjectionMatrix(light->projectionMatrix);
							}

							break;
						}

						default:
						{
							R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);
							break;
						}
					}
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Shadow Interaction: %i -----\n", iaCount));
				}
			}
			else
			{
				GLimp_LogComment("--- Rendering lighting ---\n");

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Light Interaction: %i -----\n", iaCount));
				}

				// finally draw light
				if(DS_PREPASS_LIGHTING_ENABLED())
				{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
					R_BindFBO(tr.lightRenderFBO);
#else
					R_BindNullFBO();
#endif
				}
				else
				{
					R_BindFBO(tr.deferredRenderFBO);
				}

				GLimp_LogComment("--- Rendering light volume ---\n");

				// restore camera matrices
				GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
				GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

				// set the window clipping
				GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
						   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				GL_BindProgram(&tr.genericSingleShader);
				GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
				GL_Cull(CT_TWO_SIDED);

				// set uniforms
				GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
				GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
				GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
				if(glConfig.vboVertexSkinningAvailable)
				{
					GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
				}
				GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
				GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

				// bind u_ColorMap
				GL_SelectTexture(0);
				GL_Bind(tr.whiteImage);
				GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

				// set light scissor to reduce fillrate
				//GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

				// set the reference stencil value
				GL_ClearStencil(128);

				// reset stencil buffer
				qglClear(GL_STENCIL_BUFFER_BIT);

				// use less compare as depthfunc
				// don't write to the color buffer or depth buffer
				// enable stencil testing for this light
				GL_State(GLS_DEPTHFUNC_LESS | GLS_COLORMASK_BITS | GLS_STENCILTEST_ENABLE);

				qglStencilFunc(GL_ALWAYS, 128, 255);
				qglStencilMask(255);

#if 1
				if(light->isStatic && light->frustumVBO && light->frustumIBO)
				{
					// render in world space
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					R_BindVBO(light->frustumVBO);
					R_BindIBO(light->frustumIBO);

					GL_VertexAttribsState(ATTR_POSITION);

					tess.numVertexes = light->frustumVerts;
					tess.numIndexes = light->frustumIndexes;
				}
				else
#endif
				{
					// render in light space
					R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
					GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					tess.numIndexes = 0;
					tess.numVertexes = 0;

					switch (light->l.rlType)
					{
						case RL_OMNI:
						case RL_DIRECTIONAL:
						{
							Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], colorWhite);

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							break;
						}

						case RL_PROJ:
						{
							vec3_t          farCorners[4];
							vec4_t         *frustum = light->localFrustum;

							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

							if(!VectorCompare(light->l.projStart, vec3_origin))
							{
								vec3_t          nearCorners[4];

								// calculate the vertices defining the top area
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

								// draw outer surfaces
								for(j = 0; j < 4; j++)
								{
									VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
									VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
									VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
									VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
									Tess_AddQuadStamp2(quadVerts, colorCyan);
								}

								// draw far cap
								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorRed);

								// draw near cap
								VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
								VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
								VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
								VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorGreen);

							}
							else
							{
								vec3_t	top;

								// no light_start, just use the top vertex (doesn't need to be mirrored)
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

								// draw pyramid
								for(j = 0; j < 4; j++)
								{
									VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(top, tess.xyz[tess.numVertexes]);
									VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;
								}

								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorRed);
							}

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							break;
						}

						default:
							break;
					}
				}

				// render in world space
				//backEnd.orientation = backEnd.viewParms.world;

				if(r_showShadowVolumes->integer)
				{
					GL_State(GLS_DEPTHFUNC_LESS | GLS_POLYMODE_LINE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
					GL_Cull(CT_FRONT_SIDED);
					Tess_DrawElements();
				}
				else
				{
					if(qglStencilFuncSeparateATI && qglStencilOpSeparateATI && glConfig.stencilWrapAvailable)
					{
						GL_Cull(CT_TWO_SIDED);

						qglStencilFuncSeparateATI(GL_ALWAYS, GL_ALWAYS, 0, (GLuint) ~ 0);

						qglStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						qglStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);

						Tess_DrawElements();
					}
					else if(qglActiveStencilFaceEXT)
					{
						// render both sides at once
						GL_Cull(CT_TWO_SIDED);

						qglEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);

						qglActiveStencilFaceEXT(GL_BACK);
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
						}

						qglActiveStencilFaceEXT(GL_FRONT);
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
						}

						Tess_DrawElements();

						qglDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
					}
					else
					{
						// draw only the front faces of the shadow volume
						GL_Cull(CT_FRONT_SIDED);

						// increment the stencil value on zfail
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
						}

						Tess_DrawElements();

						// draw only the back faces of the shadow volume
						GL_Cull(CT_BACK_SIDED);

						// decrement the stencil value on zfail
						if(glConfig.stencilWrapAvailable)
						{
							qglStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
						}
						else
						{
							qglStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
						}

						Tess_DrawElements();
					}
				}

				GL_CheckErrors();

				GLimp_LogComment("--- Rendering lighting ---\n");

				qglStencilFunc(GL_NOTEQUAL, 128, 255);

				if(qglActiveStencilFaceEXT)
				{
					qglActiveStencilFaceEXT(GL_BACK);
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

					qglActiveStencilFaceEXT(GL_FRONT);
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}
				else
				{
					qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				}

				GL_CheckErrors();

				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						// reset light view and projection matrices
						MatrixAffineInverse(light->transformMatrix, light->viewMatrix);
						MatrixSetupScale(light->projectionMatrix, 1.0 / light->l.radius[0], 1.0 / light->l.radius[1],
										 1.0 / light->l.radius[2]);

						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);		// scale
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);

						MatrixCopy(light->attenuationMatrix, light->shadowMatrices[0]);
						break;
					}

					case RL_PROJ:
					{
						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);

						MatrixCopy(light->attenuationMatrix, light->shadowMatrices[0]);
						break;
					}

					case RL_DIRECTIONAL:
					{
						matrix_t        viewMatrix, projectionMatrix;

						// build same attenuation matrix as for box lights so we can clip pixels outside of the light volume
						MatrixAffineInverse(light->transformMatrix, viewMatrix);
						MatrixSetupScale(projectionMatrix, 1.0 / light->l.radius[0], 1.0 / light->l.radius[1], 1.0 / light->l.radius[2]);

						MatrixCopy(bias, light->attenuationMatrix);
						MatrixMultiply2(light->attenuationMatrix, projectionMatrix);
						MatrixMultiply2(light->attenuationMatrix, viewMatrix);
						break;
					}

					default:
						break;
				}

				// update uniforms
				VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);

				// set 2D virtual screen size
				GL_PushMatrix();

				MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
												backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
												backEnd.viewParms.viewportY,
												backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999);
				GL_LoadProjectionMatrix(ortho);
				GL_LoadModelViewMatrix(matrixIdentity);

				// last interaction of current light
				lightShader = light->shader;
				attenuationZStage = lightShader->stages[0];

				for(j = 1; j < MAX_SHADER_STAGES; j++)
				{
					attenuationXYStage = lightShader->stages[j];

					if(!attenuationXYStage)
					{
						break;
					}

					if(attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
					{
						continue;
					}

					if(!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
					{
						continue;
					}

					Tess_ComputeColor(attenuationXYStage);
					R_ComputeFinalAttenuation(attenuationXYStage, light);

					if(light->l.rlType == RL_OMNI)
					{
						// enable shader, set arrays
						GL_BindProgram(&tr.deferredLightingShader_DBS_omni);

						// set OpenGL state for additive lighting
						GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);

						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);
						shadowCompare = !light->l.noShadows && light->shadowLOD >= 0;

						GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_omni, viewOrigin);
						GLSL_SetUniform_LightOrigin(&tr.deferredLightingShader_DBS_omni, lightOrigin);
						GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_omni, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_omni, light->sphereRadius);
						GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_omni, light->l.scale);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_omni, light->attenuationMatrix2);

						GLSL_SetUniform_ShadowCompare(&tr.deferredLightingShader_DBS_omni, shadowCompare);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_omni, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_omni, backEnd.viewParms.unprojectionMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_omni, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_omni, plane);
						}

						if(DS_STANDARD_ENABLED())
						{
							// bind u_DiffuseMap
							GL_SelectTexture(0);
							GL_Bind(tr.deferredDiffuseFBOImage);
						}

						// bind u_NormalMap
						GL_SelectTexture(1);
						GL_Bind(tr.deferredNormalFBOImage);

						if(DS_STANDARD_ENABLED() && r_normalMapping->integer)
						{
							// bind u_SpecularMap
							GL_SelectTexture(2);
							GL_Bind(tr.deferredSpecularFBOImage);
						}

						// bind u_DepthMap
						GL_SelectTexture(3);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(4);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(5);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

						// bind u_ShadowMap
						if(shadowCompare)
						{
							GL_SelectTexture(6);
							GL_Bind(tr.shadowCubeFBOImage[light->shadowLOD]);
						}

#if 0
						// draw lighting with a fullscreen quad
						Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
#endif
					}
					else if(light->l.rlType == RL_PROJ)
					{
#if 0
						if(light->l.inverseShadows)
						{
							GL_BindProgram(&tr.deferredShadowingShader_proj);

							GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR | GLS_STENCILTEST_ENABLE);
							//GL_State(GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA);
							GL_Cull(CT_TWO_SIDED);

							// set uniforms
							VectorCopy(light->origin, lightOrigin);
							VectorCopy(tess.svars.color, lightColor);
							shadowCompare = !light->l.noShadows && light->shadowLOD >= 0;

							GLSL_SetUniform_LightOrigin(&tr.deferredShadowingShader_proj, lightOrigin);
							GLSL_SetUniform_LightColor(&tr.deferredShadowingShader_proj, lightColor);
							GLSL_SetUniform_LightRadius(&tr.deferredShadowingShader_proj, light->sphereRadius);
							GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredShadowingShader_proj, light->attenuationMatrix2);

							// FIXME GLSL_SetUniform_ShadowMatrix(&tr.deferredShadowingShader_proj, light->attenuationMatrix);
							GLSL_SetUniform_ShadowCompare(&tr.deferredShadowingShader_proj, shadowCompare);

							GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredShadowingShader_proj, glState.modelViewProjectionMatrix[glState.stackIndex]);
							GLSL_SetUniform_UnprojectMatrix(&tr.deferredShadowingShader_proj, backEnd.viewParms.unprojectionMatrix);

							GLSL_SetUniform_PortalClipping(&tr.deferredShadowingShader_proj, backEnd.viewParms.isPortal);
							if(backEnd.viewParms.isPortal)
							{
								float           plane[4];

								// clipping plane in world space
								plane[0] = backEnd.viewParms.portalPlane.normal[0];
								plane[1] = backEnd.viewParms.portalPlane.normal[1];
								plane[2] = backEnd.viewParms.portalPlane.normal[2];
								plane[3] = backEnd.viewParms.portalPlane.dist;

								GLSL_SetUniform_PortalPlane(&tr.deferredShadowingShader_proj, plane);
							}

							// bind u_DepthMap
							GL_SelectTexture(0);
							GL_Bind(tr.depthRenderImage);

							// bind u_AttenuationMapXY
							GL_SelectTexture(1);
							BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

							// bind u_AttenuationMapZ
							GL_SelectTexture(2);
							BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

							// bind u_ShadowMap
							if(shadowCompare)
							{
								GL_SelectTexture(3);
								GL_Bind(tr.shadowMapFBOImage[light->shadowLOD]);
							}

							// draw lighting with a fullscreen quad
							Tess_InstantQuad(backEnd.viewParms.viewportVerts);
						}
						else
#endif
						{
							GL_BindProgram(&tr.deferredLightingShader_DBS_proj);

							// set OpenGL state for additive lighting
							GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);
							GL_Cull(CT_TWO_SIDED);

							// set uniforms
							VectorCopy(light->origin, lightOrigin);
							VectorCopy(tess.svars.color, lightColor);
							shadowCompare = !light->l.noShadows && light->shadowLOD >= 0;

							GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_proj, viewOrigin);
							GLSL_SetUniform_LightOrigin(&tr.deferredLightingShader_DBS_proj, lightOrigin);
							GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_proj, lightColor);
							GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_proj, light->sphereRadius);
							GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_proj, light->l.scale);
							GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_proj, light->attenuationMatrix2);

							GLSL_SetUniform_ShadowMatrix(&tr.deferredLightingShader_DBS_proj, light->shadowMatrices);
							GLSL_SetUniform_ShadowCompare(&tr.deferredLightingShader_DBS_proj, shadowCompare);

							GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_proj, glState.modelViewProjectionMatrix[glState.stackIndex]);
							GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_proj, backEnd.viewParms.unprojectionMatrix);

							GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_proj, backEnd.viewParms.isPortal);
							if(backEnd.viewParms.isPortal)
							{
								float           plane[4];

								// clipping plane in world space
								plane[0] = backEnd.viewParms.portalPlane.normal[0];
								plane[1] = backEnd.viewParms.portalPlane.normal[1];
								plane[2] = backEnd.viewParms.portalPlane.normal[2];
								plane[3] = backEnd.viewParms.portalPlane.dist;

								GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_proj, plane);
							}

							if(DS_STANDARD_ENABLED())
							{
								// bind u_DiffuseMap
								GL_SelectTexture(0);
								GL_Bind(tr.deferredDiffuseFBOImage);
							}

							// bind u_NormalMap
							GL_SelectTexture(1);
							GL_Bind(tr.deferredNormalFBOImage);

							if(DS_STANDARD_ENABLED() && r_normalMapping->integer)
							{
								// bind u_SpecularMap
								GL_SelectTexture(2);
								GL_Bind(tr.deferredSpecularFBOImage);
							}

							// bind u_DepthMap
							GL_SelectTexture(3);
							GL_Bind(tr.depthRenderImage);

							// bind u_AttenuationMapXY
							GL_SelectTexture(4);
							BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

							// bind u_AttenuationMapZ
							GL_SelectTexture(5);
							BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

							// bind u_ShadowMap
							if(shadowCompare)
							{
								GL_SelectTexture(6);
								GL_Bind(tr.shadowMapFBOImage[light->shadowLOD]);
							}

#if 0
							// draw lighting with a fullscreen quad
							Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
							VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
							VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
							VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Tess_InstantQuad(quadVerts);
#endif
						}
					}
					else if(light->l.rlType == RL_DIRECTIONAL)
					{
						// enable shader, set arrays
						GL_BindProgram(&tr.deferredLightingShader_DBS_directional);

						// set OpenGL state for additive lighting
						GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE | GLS_STENCILTEST_ENABLE);

						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);
						shadowCompare = qtrue; //!light->l.noShadows && light->shadowLOD >= 0;

						GLSL_SetUniform_ViewOrigin(&tr.deferredLightingShader_DBS_directional, viewOrigin);

#if 1
						VectorCopy(tr.sunDirection, lightDirection);
#else
						VectorCopy(light->direction, lightDirection);
#endif
						GLSL_SetUniform_LightDir(&tr.deferredLightingShader_DBS_directional, lightDirection);

						GLSL_SetUniform_LightColor(&tr.deferredLightingShader_DBS_directional, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredLightingShader_DBS_directional, light->sphereRadius);
						GLSL_SetUniform_LightScale(&tr.deferredLightingShader_DBS_directional, light->l.scale);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredLightingShader_DBS_directional, light->attenuationMatrix2);

						GLSL_SetUniform_ShadowMatrix(&tr.deferredLightingShader_DBS_directional, light->shadowMatricesBiased);
						GLSL_SetUniform_ShadowCompare(&tr.deferredLightingShader_DBS_directional, shadowCompare);
						GLSL_SetUniform_ShadowParallelSplitDistances(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.parallelSplitDistances);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredLightingShader_DBS_directional, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.unprojectionMatrix);
						GLSL_SetUniform_ViewMatrix(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.world.viewMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredLightingShader_DBS_directional, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredLightingShader_DBS_directional, plane);
						}

						if(DS_STANDARD_ENABLED())
						{
							// bind u_DiffuseMap
							GL_SelectTexture(0);
							GL_Bind(tr.deferredDiffuseFBOImage);
						}

						// bind u_NormalMap
						GL_SelectTexture(1);
						GL_Bind(tr.deferredNormalFBOImage);

						if(DS_STANDARD_ENABLED() && r_normalMapping->integer)
						{
							// bind u_SpecularMap
							GL_SelectTexture(2);
							GL_Bind(tr.deferredSpecularFBOImage);
						}

						// bind u_DepthMap
						GL_SelectTexture(3);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(4);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(5);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

						// bind shadow maps
						if(shadowCompare)
						{
							GL_SelectTexture(6);
							GL_Bind(tr.shadowMapFBOImage[0]);

							if(r_parallelShadowSplits->integer >= 1)
							{
								GL_SelectTexture(7);
								GL_Bind(tr.shadowMapFBOImage[1]);
							}

							if(r_parallelShadowSplits->integer >= 2)
							{
								GL_SelectTexture(8);
								GL_Bind(tr.shadowMapFBOImage[2]);
							}

							if(r_parallelShadowSplits->integer >= 3)
							{
								GL_SelectTexture(9);
								GL_Bind(tr.shadowMapFBOImage[3]);
							}

							if(r_parallelShadowSplits->integer >= 4)
							{
								GL_SelectTexture(10);
								GL_Bind(tr.shadowMapFBOImage[4]);
							}
						}

#if 0
						// draw lighting with a fullscreen quad
						Tess_InstantQuad(backEnd.viewParms.viewportVerts);
#else
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
#endif
					}
				}

				// draw split frustum shadow maps
				if(r_showShadowMaps->integer && light->l.rlType == RL_DIRECTIONAL)
				{
					int			frustumIndex;
					float		x, y, w, h;

					GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
								backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

					GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

					for(frustumIndex = 0; frustumIndex <= r_parallelShadowSplits->integer; frustumIndex++)
					{
						GL_BindProgram(&tr.debugShadowMapShader);
						GL_Cull(CT_TWO_SIDED);
						GL_State(GLS_DEPTHTEST_DISABLE);

						// set uniforms
						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.debugShadowMapShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

						GL_SelectTexture(0);
						GL_Bind(tr.shadowMapFBOImage[frustumIndex]);

						w = 200;
						h = 200;

						x = 205 * frustumIndex;
						y = 70;

						VectorSet4(quadVerts[0], x, y, 0, 1);
						VectorSet4(quadVerts[1], x + w, y, 0, 1);
						VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
						VectorSet4(quadVerts[3], x, y + h, 0, 1);

						Tess_InstantQuad(quadVerts);

						{
							int				j;
							vec4_t			splitFrustum[6];
							vec3_t          farCorners[4];
							vec3_t          nearCorners[4];

							GL_Viewport(x, y, w, h);
							GL_Scissor(x, y, w, h);

							GL_PushMatrix();

							GL_BindProgram(&tr.genericSingleShader);
							GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
							GL_Cull(CT_TWO_SIDED);

							// set uniforms
							GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
							GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
							GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
							if(glConfig.vboVertexSkinningAvailable)
							{
								GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
							}
							GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
							GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

							// bind u_ColorMap
							GL_SelectTexture(0);
							GL_Bind(tr.whiteImage);
							GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

							GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, light->shadowMatrices[frustumIndex]);

							tess.numIndexes = 0;
							tess.numVertexes = 0;

#if 1
							for(j = 0; j < 6; j++)
							{
								VectorCopy(backEnd.viewParms.frustums[1 + frustumIndex][j].normal, splitFrustum[j]);
								splitFrustum[j][3] = backEnd.viewParms.frustums[1 + frustumIndex][j].dist;
							}
#else
							for(j = 0; j < 6; j++)
							{
								VectorCopy(backEnd.viewParms.frustums[0][j].normal, splitFrustum[j]);
								splitFrustum[j][3] = backEnd.viewParms.frustums[0][j].dist;
							}
#endif

							// calculate split frustum corner points
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[0]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_NEAR], nearCorners[1]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[2]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_NEAR], nearCorners[3]);

							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[0]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_TOP], splitFrustum[FRUSTUM_FAR], farCorners[1]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_RIGHT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[2]);
							PlanesGetIntersectionPoint(splitFrustum[FRUSTUM_LEFT], splitFrustum[FRUSTUM_BOTTOM], splitFrustum[FRUSTUM_FAR], farCorners[3]);

							// draw outer surfaces
#if 1
							for(j = 0; j < 4; j++)
							{
								VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
								VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
								VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
								VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
								Tess_AddQuadStamp2(quadVerts, colorCyan);
							}

							// draw far cap
							VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
							VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
							VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
							VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
							Tess_AddQuadStamp2(quadVerts, colorBlue);

							// draw near cap
							VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
							VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
							VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
							VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
							Tess_AddQuadStamp2(quadVerts, colorGreen);
#else

							// draw pyramid
							for(j = 0; j < 4; j++)
							{
								VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
								VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
								VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy(backEnd.viewParms.orientation.origin, tess.xyz[tess.numVertexes]);
								VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
								tess.indexes[tess.numIndexes++] = tess.numVertexes;
								tess.numVertexes++;
							}

							VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
							VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
							VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
							VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
							Tess_AddQuadStamp2(quadVerts, colorRed);
#endif

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							Tess_DrawElements();

							// draw light volume
							if(light->isStatic && light->frustumVBO && light->frustumIBO)
							{
								GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_CUSTOM_RGB);
								GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_CUSTOM);
								GLSL_SetUniform_Color(&tr.genericSingleShader, colorYellow);

								R_BindVBO(light->frustumVBO);
								R_BindIBO(light->frustumIBO);

								GL_VertexAttribsState(ATTR_POSITION);

								tess.numVertexes = light->frustumVerts;
								tess.numIndexes = light->frustumIndexes;

								Tess_DrawElements();
							}

							tess.numIndexes = 0;
							tess.numVertexes = 0;

							GL_PopMatrix();

							GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
										backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

							GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
									   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
						}
					}
				}

				// end of lighting
				GL_PopMatrix();
			}
		}						// end if(iaCount == iaFirst)

		if(drawShadows)
		{
			if(entity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			{
				goto skipInteraction;
			}

			if(shader->isSky)
			{
				goto skipInteraction;
			}

			if(shader->sort > SS_OPAQUE)
			{
				goto skipInteraction;
			}

			if(shader->noShadows || light->l.noShadows || light->shadowLOD < 0)
			{
				goto skipInteraction;
			}

			if(light->l.inverseShadows && (entity == &tr.worldEntity))
			{
				// this light only casts shadows by its player and their items
				goto skipInteraction;
			}

			if(ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}

			if(light->l.rlType == RL_OMNI && !(ia->cubeSideBits & (1 << cubeSide)))
			{
				goto skipInteraction;
			}

			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
				{
#if 1
					if((iaCount != iaFirst) &&
						light == oldLight &&
						entity == oldEntity &&
						(alphaTest ? shader == oldShader : alphaTest == oldAlphaTest) &&
						(deformType == oldDeformType))
					{
						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Batching Shadow Interaction: %i -----\n", iaCount));
						}

						// fast path, same as previous
						rb_surfaceTable[*surface] (surface);
						goto nextInteraction;
					}
					else
#endif
					{
						if(oldLight)
						{
							// draw the contents of the last shader batch
							Tess_End();
						}

						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Beginning Shadow Interaction: %i -----\n", iaCount));
						}

						// we don't need tangent space calculations here
						Tess_Begin(Tess_StageIteratorShadowFill, shader, light->shader, qtrue, qfalse, -1);
					}
					break;
				}

				default:
					break;
			}
		}
		else
		{
			// jump to !ia->next
			goto nextInteraction;
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					R_RotateEntityForLight(entity, light, &backEnd.orientation);
				}
				else
				{
					R_RotateEntityForViewParms(entity, &backEnd.viewParms, &backEnd.orientation);
				}

				if(entity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					Com_Memset(&backEnd.orientation, 0, sizeof(backEnd.orientation));

					backEnd.orientation.axis[0][0] = 1;
					backEnd.orientation.axis[1][1] = 1;
					backEnd.orientation.axis[2][2] = 1;
					VectorCopy(light->l.origin, backEnd.orientation.viewOrigin);

					MatrixIdentity(backEnd.orientation.transformMatrix);
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixCopy(backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix);
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		if(drawShadows)
		{
			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
				{
					// add the triangles for this surface
					rb_surfaceTable[*surface] (surface);
					break;
				}

				default:
					break;
			}
		}
		else
		{
			// DO NOTHING
			//rb_surfaceTable[*surface] (surface, ia->numLightIndexes, ia->lightIndexes, 0, NULL);
		}

	  nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;
		oldAlphaTest = alphaTest;
		oldDeformType = deformType;

	  skipInteraction:
		if(!ia->next)
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if(r_logFile->integer)
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment(va("----- Last Interaction: %i -----\n", iaCount));
			}

			if(drawShadows)
			{
				// draw the contents of the last shader batch
				Tess_End();

				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						if(cubeSide == 5)
						{
							cubeSide = 0;
							drawShadows = qfalse;
						}
						else
						{
							cubeSide++;
						}

						// jump back to first interaction of this light
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						break;
					}

					case RL_PROJ:
					{
						// jump back to first interaction of this light and start lighting
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						drawShadows = qfalse;
						break;
					}

					case RL_DIRECTIONAL:
					{
						// set shadow matrix including scale + offset
						MatrixCopy(bias, light->shadowMatricesBiased[splitFrustumIndex]);
						MatrixMultiply2(light->shadowMatricesBiased[splitFrustumIndex], light->projectionMatrix);
						MatrixMultiply2(light->shadowMatricesBiased[splitFrustumIndex], light->viewMatrix);

						MatrixMultiply(light->projectionMatrix, light->viewMatrix, light->shadowMatrices[splitFrustumIndex]);

						if(r_parallelShadowSplits->integer)
						{
							if(splitFrustumIndex == r_parallelShadowSplits->integer)
							{
								splitFrustumIndex = 0;
								drawShadows = qfalse;
							}
							else
							{
								splitFrustumIndex++;
							}

							// jump back to first interaction of this light
							ia = &backEnd.viewParms.interactions[iaFirst];
							iaCount = iaFirst;
						}
						else
						{
							// jump back to first interaction of this light and start lighting
							ia = &backEnd.viewParms.interactions[iaFirst];
							iaCount = iaFirst;
							drawShadows = qfalse;
						}
						break;
					}

					default:
						break;
				}
			}
			else
			{
#ifdef VOLUMETRIC_LIGHTING
				// draw the light volume if needed
				if(light->shader->volumetricLight)
				{
					Render_lightVolume(ia);
				}
#endif

				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// reset scissor clamping
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// reset clear color
	GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredLightingTime = endTime - startTime;
	}
}

#if 0
static void RB_RenderInteractionsDeferredInverseShadows()
{
	interaction_t  *ia;
	int             iaCount;
	int             iaFirst;
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	surfaceType_t  *surface;
	qboolean        depthRange, oldDepthRange;
	qboolean        alphaTest, oldAlphaTest;
	qboolean        drawShadows;
	int             cubeSide;

	shader_t       *lightShader;
	shaderStage_t  *attenuationXYStage;
	shaderStage_t  *attenuationZStage;
	int             i, j;
	vec3_t          viewOrigin;
	vec3_t          lightOrigin;
	vec4_t          lightColor;
	vec4_t          lightFrustum[6];
	cplane_t       *frust;
	qboolean        shadowCompare;
	matrix_t        ortho;
	vec4_t          quadVerts[4];
	int             startTime = 0, endTime = 0;

	GLimp_LogComment("--- RB_RenderInteractionsDeferredInverseShadows ---\n");

	if(!glConfig.framebufferObjectAvailable)
		return;

	if(r_hdrRendering->integer && !glConfig.textureFloatAvailable)
		return;

	if(r_speeds->integer == 9)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	drawShadows = qtrue;
	cubeSide = 0;

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// update depth render image
	if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
					   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
	{
		// no update needed FBO handles it
		R_BindFBO(tr.deferredRenderFBO);
	}
	else if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
		// no update needed FBO handles it
		R_BindFBO(tr.deferredRenderFBO);
	}
	else
	{
		R_BindNullFBO();
		GL_SelectTexture(0);
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
	}


	// render interactions
	for(iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;
		alphaTest = shader->alphaTest;

		if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !ia->occlusionQuerySamples)
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		if(!light->l.inverseShadows)
		{
			// we only care about inverse shadows as this is a post process effect
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if(iaCount == iaFirst)
		{
			if(drawShadows)
			{
				// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
				GL_BindProgram(NULL);
				GL_State(GLS_DEFAULT);
				//GL_VertexAttribsState(ATTR_POSITION);

				GL_SelectTexture(0);
				GL_Bind(tr.whiteImage);

				if(light->l.noShadows || light->shadowLOD < 0)
				{
					if(r_logFile->integer)
					{
						// don't just call LogComment, or we will get
						// a call to va() every frame!
						GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));
					}

					goto skipInteraction;
				}
				else
				{
					R_BindFBO(tr.shadowMapFBO[light->shadowLOD]);

					switch (light->l.rlType)
					{
						case RL_OMNI:
						{
							float           xMin, xMax, yMin, yMax;
							float           width, height, depth;
							float           zNear, zFar;
							float           fovX, fovY;
							qboolean        flipX, flipY;
							float          *proj;
							vec3_t          angles;
							matrix_t        rotationMatrix, transformMatrix, viewMatrix;

							if(r_logFile->integer)
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment(va("----- Rendering shadowCube side: %i -----\n", cubeSide));
							}

							R_AttachFBOTexture2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + cubeSide,
												 tr.shadowCubeFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							switch (cubeSide)
							{
								case 0:
								{
									// view parameters
									VectorSet(angles, 0, 0, 90);

									// projection parameters
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 1:
								{
									VectorSet(angles, 0, 180, 90);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 2:
								{
									VectorSet(angles, 0, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 3:
								{
									VectorSet(angles, 0, -90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								case 4:
								{
									VectorSet(angles, -90, 90, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}

								case 5:
								{
									VectorSet(angles, 90, 90, 0);
									flipX = qtrue;
									flipY = qtrue;
									break;
								}

								default:
								{
									// shut up compiler
									VectorSet(angles, 0, 0, 0);
									flipX = qfalse;
									flipY = qfalse;
									break;
								}
							}

							// Quake -> OpenGL view matrix from light perspective
							MatrixFromAngles(rotationMatrix, angles[PITCH], angles[YAW], angles[ROLL]);
							MatrixSetupTransformFromRotation(transformMatrix, rotationMatrix, light->origin);
							MatrixAffineInverse(transformMatrix, viewMatrix);

							// convert from our coordinate system (looking down X)
							// to OpenGL's coordinate system (looking down -Z)
							MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);

							// OpenGL projection matrix
							fovX = 90;
							fovY = 90;	//R_CalcFov(fovX, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							zNear = 1.0;
							zFar = light->sphereRadius;

							if(!flipX)
							{
								xMax = zNear * tan(fovX * M_PI / 360.0f);
								xMin = -xMax;
							}
							else
							{
								xMin = zNear * tan(fovX * M_PI / 360.0f);
								xMax = -xMin;
							}

							if(!flipY)
							{
								yMax = zNear * tan(fovY * M_PI / 360.0f);
								yMin = -yMax;
							}
							else
							{
								yMin = zNear * tan(fovY * M_PI / 360.0f);
								yMax = -yMin;
							}

							width = xMax - xMin;
							height = yMax - yMin;
							depth = zFar - zNear;

							proj = light->projectionMatrix;
							proj[0] = (2 * zNear) / width;
							proj[4] = 0;
							proj[8] = (xMax + xMin) / width;
							proj[12] = 0;
							proj[1] = 0;
							proj[5] = (2 * zNear) / height;
							proj[9] = (yMax + yMin) / height;
							proj[13] = 0;
							proj[2] = 0;
							proj[6] = 0;
							proj[10] = -(zFar + zNear) / depth;
							proj[14] = -(2 * zFar * zNear) / depth;
							proj[3] = 0;
							proj[7] = 0;
							proj[11] = -1;
							proj[15] = 0;

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						case RL_PROJ:
						{
							GLimp_LogComment("--- Rendering projective shadowMap ---\n");

							R_AttachFBOTexture2D(GL_TEXTURE_2D, tr.shadowMapFBOImage[light->shadowLOD]->texnum, 0);
							if(!r_ignoreGLErrors->integer)
							{
								R_CheckFBO(tr.shadowMapFBO[light->shadowLOD]);
							}

							// set the window clipping
							GL_Viewport(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);
							GL_Scissor(0, 0, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

							qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

							GL_LoadProjectionMatrix(light->projectionMatrix);
							break;
						}

						default:
							break;
					}
				}

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Shadow Interaction: %i -----\n", iaCount));
				}
			}
			else
			{
				GLimp_LogComment("--- Rendering lighting ---\n");

				if(r_logFile->integer)
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment(va("----- First Light Interaction: %i -----\n", iaCount));
				}

				// finally draw light
				R_BindFBO(tr.deferredRenderFBO);

				// set the window clipping
				GL_Viewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				//GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				//        backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

				// set light scissor to reduce fillrate
				GL_Scissor(ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight);

				// restore camera matrices
				GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
				GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						// reset light view and projection matrices
						MatrixAffineInverse(light->transformMatrix, light->viewMatrix);
						MatrixSetupScale(light->projectionMatrix, 1.0 / light->l.radius[0], 1.0 / light->l.radius[1],
										 1.0 / light->l.radius[2]);

						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.5);	// bias
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 0.5);	// scale
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);	// light projection (frustum)
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);
						break;
					}

					case RL_PROJ:
					{
						// build the attenuation matrix
						MatrixSetupTranslation(light->attenuationMatrix, 0.5, 0.5, 0.0);	// bias
						MatrixMultiplyScale(light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min(light->falloffLength, 1.0));	// scale
						MatrixMultiply2(light->attenuationMatrix, light->projectionMatrix);
						MatrixMultiply2(light->attenuationMatrix, light->viewMatrix);
						break;
					}

					default:
						break;
				}

				// update uniforms
				VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);

				// copy frustum planes for pixel shader
				for(i = 0; i < 6; i++)
				{
					frust = &light->frustum[i];

					VectorCopy(frust->normal, lightFrustum[i]);
					lightFrustum[i][3] = frust->dist;
				}

				// set 2D virtual screen size
				GL_PushMatrix();
				MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
												backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
												backEnd.viewParms.viewportY,
												backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999);
				GL_LoadProjectionMatrix(ortho);
				GL_LoadModelViewMatrix(matrixIdentity);

				// last interaction of current light
				lightShader = light->shader;
				attenuationZStage = lightShader->stages[0];

				for(j = 1; j < MAX_SHADER_STAGES; j++)
				{
					attenuationXYStage = lightShader->stages[j];

					if(!attenuationXYStage)
					{
						break;
					}

					if(attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
					{
						continue;
					}

					if(!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
					{
						continue;
					}

					Tess_ComputeColor(attenuationXYStage);
					R_ComputeFinalAttenuation(attenuationXYStage, light);

					if(light->l.rlType == RL_OMNI)
					{
						// TODO
					}
					else if(light->l.rlType == RL_PROJ)
					{

						GL_BindProgram(&tr.deferredShadowingShader_proj);

						GL_State(GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
						GL_Cull(CT_TWO_SIDED);

						// set uniforms
						VectorCopy(light->origin, lightOrigin);
						VectorCopy(tess.svars.color, lightColor);
						shadowCompare = !light->l.noShadows && light->shadowLOD >= 0;

						GLSL_SetUniform_LightOrigin(&tr.deferredShadowingShader_proj, lightOrigin);
						GLSL_SetUniform_LightColor(&tr.deferredShadowingShader_proj, lightColor);
						GLSL_SetUniform_LightRadius(&tr.deferredShadowingShader_proj, light->sphereRadius);
						GLSL_SetUniform_LightAttenuationMatrix(&tr.deferredShadowingShader_proj, light->attenuationMatrix2);

						GLSL_SetUniform_ShadowMatrix(&tr.deferredShadowingShader_proj, light->shadowMatrices);
						GLSL_SetUniform_ShadowCompare(&tr.deferredShadowingShader_proj, shadowCompare);

						GLSL_SetUniform_ModelViewProjectionMatrix(&tr.deferredShadowingShader_proj, glState.modelViewProjectionMatrix[glState.stackIndex]);
						GLSL_SetUniform_UnprojectMatrix(&tr.deferredShadowingShader_proj, backEnd.viewParms.unprojectionMatrix);

						GLSL_SetUniform_PortalClipping(&tr.deferredShadowingShader_proj, backEnd.viewParms.isPortal);
						if(backEnd.viewParms.isPortal)
						{
							float           plane[4];

							// clipping plane in world space
							plane[0] = backEnd.viewParms.portalPlane.normal[0];
							plane[1] = backEnd.viewParms.portalPlane.normal[1];
							plane[2] = backEnd.viewParms.portalPlane.normal[2];
							plane[3] = backEnd.viewParms.portalPlane.dist;

							GLSL_SetUniform_PortalPlane(&tr.deferredShadowingShader_proj, plane);
						}

						// bind u_DepthMap
						GL_SelectTexture(0);
						GL_Bind(tr.depthRenderImage);

						// bind u_AttenuationMapXY
						GL_SelectTexture(1);
						BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						GL_SelectTexture(2);
						BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

						// bind u_ShadowMap
						if(shadowCompare)
						{
							GL_SelectTexture(3);
							GL_Bind(tr.shadowMapFBOImage[light->shadowLOD]);
						}

						// draw lighting
						VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
						VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
						Tess_InstantQuad(quadVerts);
					}
					else
					{
						// TODO
					}

					// end of lighting
					GL_PopMatrix();

					R_BindNullFBO();
				}
			}
		}						// end if(iaCount == iaFirst)

		if(drawShadows)
		{
			if(entity->e.renderfx & (RF_NOSHADOW | RF_DEPTHHACK))
			{
				goto skipInteraction;
			}

			if(shader->isSky)
			{
				goto skipInteraction;
			}

			if(shader->sort > SS_OPAQUE)
			{
				goto skipInteraction;
			}

			if(shader->noShadows || light->l.noShadows || light->shadowLOD < 0)
			{
				goto skipInteraction;
			}

			if(light->l.inverseShadows && (entity == &tr.worldEntity))
			{
				// this light only casts shadows by its player and their items
				goto skipInteraction;
			}

			if(ia->type == IA_LIGHTONLY)
			{
				goto skipInteraction;
			}

			if(light->l.rlType == RL_OMNI && !(ia->cubeSideBits & (1 << cubeSide)))
			{
				goto skipInteraction;
			}

			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				{
					if(light == oldLight && entity == oldEntity && (alphaTest ? shader == oldShader : alphaTest == oldAlphaTest))
					{
						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Batching Shadow Interaction: %i -----\n", iaCount));
						}

						// fast path, same as previous
						rb_surfaceTable[*surface] (surface);
						goto nextInteraction;
					}
					else
					{
						if(oldLight)
						{
							// draw the contents of the last shader batch
							Tess_End();
						}

						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("----- Beginning Shadow Interaction: %i -----\n", iaCount));
						}

						// we don't need tangent space calculations here
						Tess_Begin(Tess_StageIteratorShadowFill, shader, light->shader, qtrue, qfalse, -1);
					}
					break;
				}

				default:
					break;
			}
		}
		else
		{
			// jump to !ia->next
			goto nextInteraction;
		}

		// change the modelview matrix if needed
		if(entity != oldEntity)
		{
			depthRange = qfalse;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					R_RotateEntityForLight(entity, light, &backEnd.orientation);
				}
				else
				{
					R_RotateEntityForViewParms(entity, &backEnd.viewParms, &backEnd.orientation);
				}

				if(entity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if(drawShadows)
				{
					Com_Memset(&backEnd.orientation, 0, sizeof(backEnd.orientation));

					backEnd.orientation.axis[0][0] = 1;
					backEnd.orientation.axis[1][1] = 1;
					backEnd.orientation.axis[2][2] = 1;
					VectorCopy(light->l.origin, backEnd.orientation.viewOrigin);

					MatrixIdentity(backEnd.orientation.transformMatrix);
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixMultiply(light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixCopy(backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix);
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);

			// change depthrange if needed
			if(oldDepthRange != depthRange)
			{
				if(depthRange)
				{
					qglDepthRange(0, 0.3);
				}
				else
				{
					qglDepthRange(0, 1);
				}
				oldDepthRange = depthRange;
			}
		}

		if(drawShadows)
		{
			switch (light->l.rlType)
			{
				case RL_OMNI:
				case RL_PROJ:
				{
					// add the triangles for this surface
					rb_surfaceTable[*surface] (surface);
					break;
				}

				default:
					break;
			}
		}
		else
		{
			// DO NOTHING
			//rb_surfaceTable[*surface] (surface, ia->numLightIndexes, ia->lightIndexes, 0, NULL);
		}

	  nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;
		oldAlphaTest = alphaTest;

	  skipInteraction:
		if(!ia->next)
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if(r_logFile->integer)
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment(va("----- Last Interaction: %i -----\n", iaCount));
			}

			if(drawShadows)
			{
				// draw the contents of the last shader batch
				Tess_End();

				switch (light->l.rlType)
				{
					case RL_OMNI:
					{
						if(cubeSide == 5)
						{
							cubeSide = 0;
							drawShadows = qfalse;
						}
						else
						{
							cubeSide++;
						}

						// jump back to first interaction of this light
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						break;
					}

					case RL_PROJ:
					{
						// jump back to first interaction of this light and start lighting
						ia = &backEnd.viewParms.interactions[iaFirst];
						iaCount = iaFirst;
						drawShadows = qfalse;
						break;
					}

					default:
						break;
				}
			}
			else
			{
#if 0 //VOLUMETRIC_LIGHTING
				// draw the light volume if needed
				if(light->shader->volumetricLight)
				{
					Render_lightVolume(ia);
				}
#endif

				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	if(depthRange)
	{
		qglDepthRange(0, 1);
	}

	// reset scissor clamping
	GL_Scissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
			   backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);

	// reset clear color
	GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	GL_CheckErrors();

	if(r_speeds->integer == 9)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredLightingTime = endTime - startTime;
	}
}
#endif

#ifdef EXPERIMENTAL
void RB_RenderScreenSpaceAmbientOcclusion(qboolean deferred)
{
#if 0
//  int             i;
//  vec3_t          viewOrigin;
//  static vec3_t   jitter[32];
//  static qboolean jitterInit = qfalse;
//  matrix_t        projectMatrix;
	matrix_t        ortho;

	GLimp_LogComment("--- RB_RenderScreenSpaceAmbientOcclusion ---\n");

	if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	if(!r_screenSpaceAmbientOcclusion->integer)
		return;

	// enable shader, set arrays
	GL_BindProgram(&tr.screenSpaceAmbientOcclusionShader);

	GL_State(GLS_DEPTHTEST_DISABLE);	// | GLS_DEPTHMASK_TRUE);
	GL_Cull(CT_TWO_SIDED);

	qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

	// set uniforms
	/*
	   VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin); // in world space

	   if(!jitterInit)
	   {
	   for(i = 0; i < 32; i++)
	   {
	   float *jit = &jitter[i][0];

	   float rad = crandom() * 1024.0f; // FIXME radius;
	   float a = crandom() * M_PI * 2;
	   float b = crandom() * M_PI * 2;

	   jit[0] = rad * sin(a) * cos(b);
	   jit[1] = rad * sin(a) * sin(b);
	   jit[2] = rad * cos(a);
	   }

	   jitterInit = qtrue;
	   }


	   MatrixCopy(backEnd.viewParms.projectionMatrix, projectMatrix);
	   MatrixInverse(projectMatrix);

	   qglUniform3fARB(tr.screenSpaceAmbientOcclusionShader.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	   qglUniform3fvARB(tr.screenSpaceAmbientOcclusionShader.u_SSAOJitter, 32, &jitter[0][0]);
	   qglUniform1fARB(tr.screenSpaceAmbientOcclusionShader.u_SSAORadius, r_screenSpaceAmbientOcclusionRadius->value);

	   qglUniformMatrix4fvARB(tr.screenSpaceAmbientOcclusionShader.u_UnprojectMatrix, 1, GL_FALSE, backEnd.viewParms.unprojectionMatrix);
	   qglUniformMatrix4fvARB(tr.screenSpaceAmbientOcclusionShader.u_ProjectMatrix, 1, GL_FALSE, projectMatrix);
	 */

	// capture current color buffer for u_CurrentMap
	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);

	// bind u_DepthMap
	GL_SelectTexture(1);
	if(deferred)
	{
		GL_Bind(tr.deferredPositionFBOImage);
	}
	else
	{
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenSpaceAmbientOcclusionShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

	// draw viewport
	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
#endif
}
#endif
#ifdef EXPERIMENTAL
void RB_RenderDepthOfField()
{
	matrix_t        ortho;

	GLimp_LogComment("--- RB_RenderDepthOfField ---\n");

	if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	if(!r_depthOfField->integer)
		return;

	// enable shader, set arrays
	GL_BindProgram(&tr.depthOfFieldShader);

	GL_State(GLS_DEPTHTEST_DISABLE);	// | GLS_DEPTHMASK_TRUE);
	GL_Cull(CT_TWO_SIDED);

	qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

	// set uniforms

	// capture current color buffer for u_CurrentMap
	GL_SelectTexture(0);
	if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
				   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
	{
		GL_Bind(tr.deferredRenderFBOImage);
	}
	else if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
		GL_Bind(tr.deferredRenderFBOImage);
	}
	else
	{
		GL_Bind(tr.currentRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);
	}

	// bind u_DepthMap
	GL_SelectTexture(1);
	if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
			   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
	{
		GL_Bind(tr.depthRenderImage);
	}
	else if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
		GL_Bind(tr.depthRenderImage);
	}
	else
	{
		// depth texture is not bound to a FBO
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.depthOfFieldShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

	// draw viewport
	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}
#endif

void RB_RenderUniformFog()
{
	vec3_t          viewOrigin;
	float           fogDensity;
	vec3_t          fogColor;
	matrix_t        ortho;

	GLimp_LogComment("--- RB_RenderUniformFog ---\n");

	if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	if(r_noFog->integer)
		return;

	if(r_forceFog->value <= 0 && tr.fogDensity <= 0)
		return;

	if(r_forceFog->value <= 0 && VectorLength(tr.fogColor) <= 0)
		return;

	// enable shader, set arrays
	GL_BindProgram(&tr.uniformFogShader);

	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA);
	GL_Cull(CT_TWO_SIDED);

	qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

	// set uniforms
	VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);	// in world space

	if(r_forceFog->value)
	{
		fogDensity = r_forceFog->value;
		VectorCopy(colorMdGrey, fogColor);
	}
	else
	{
		fogDensity = tr.fogDensity;
		VectorCopy(tr.fogColor, fogColor);
	}

	GLSL_SetUniform_ViewOrigin(&tr.uniformFogShader, viewOrigin);
	qglUniform1fARB(tr.uniformFogShader.u_FogDensity, fogDensity);
	qglUniform3fARB(tr.uniformFogShader.u_FogColor, fogColor[0], fogColor[1], fogColor[2]);
	GLSL_SetUniform_UnprojectMatrix(&tr.uniformFogShader, backEnd.viewParms.unprojectionMatrix);

	// bind u_DepthMap
	GL_SelectTexture(0);
	if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
			   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
	{
		GL_Bind(tr.depthRenderImage);
	}
	else if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
		GL_Bind(tr.depthRenderImage);
	}
	else
	{
		// depth texture is not bound to a FBO
		GL_Bind(tr.depthRenderImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight);
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.uniformFogShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

	// draw viewport
	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

void RB_RenderBloom()
{
	int				i, j;
	matrix_t        ortho;
	matrix_t		modelView;

	GLimp_LogComment("--- RB_RenderBloom ---\n");

	if((backEnd.refdef.rdflags & (RDF_NOWORLDMODEL | RDF_NOBLOOM)) || !r_bloom->integer || backEnd.viewParms.isPortal || !glConfig.framebufferObjectAvailable)
		return;

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	MatrixIdentity(modelView);
	GL_LoadModelViewMatrix(modelView);

	// FIXME
	//if(glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10)
	{
		GL_State(GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// render contrast downscaled to 1/4th of the screen
		GL_BindProgram(&tr.contrastShader);

		GL_PushMatrix();
		GL_LoadModelViewMatrix(modelView);

#if 1
		MatrixOrthogonalProjection(ortho, 0, tr.contrastRenderFBO->width, 0, tr.contrastRenderFBO->height, -99999, 99999);
		GL_LoadProjectionMatrix(ortho);
#endif
		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.contrastShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
		GL_PopMatrix();

		if(DS_STANDARD_ENABLED() || DS_PREPASS_LIGHTING_ENABLED())
		{
			if(HDR_ENABLED())
			{
				if(r_hdrKey->value <= 0)
				{
					float			key;

					// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
					key = 1.03 - 2.0 / (2.0 + log10f(backEnd.hdrAverageLuminance + 1.0f));
					qglUniform1fARB(tr.contrastShader.u_HDRKey, key);
				}
				else
				{
					qglUniform1fARB(tr.contrastShader.u_HDRKey, r_hdrKey->value);
				}

				qglUniform1fARB(tr.contrastShader.u_HDRAverageLuminance, backEnd.hdrAverageLuminance);
				qglUniform1fARB(tr.contrastShader.u_HDRMaxLuminance, backEnd.hdrMaxLuminance);
			}

			GL_SelectTexture(0);
			GL_Bind(tr.downScaleFBOImage_quarter);
		}
		else if(HDR_ENABLED())
		{
			if(r_hdrKey->value <= 0)
			{
				float			key;

				// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
				key = 1.03 - 2.0 / (2.0 + log10f(backEnd.hdrAverageLuminance + 1.0f));
				qglUniform1fARB(tr.contrastShader.u_HDRKey, key);
			}
			else
			{
				qglUniform1fARB(tr.contrastShader.u_HDRKey, r_hdrKey->value);
			}

			qglUniform1fARB(tr.contrastShader.u_HDRAverageLuminance, backEnd.hdrAverageLuminance);
			qglUniform1fARB(tr.contrastShader.u_HDRMaxLuminance, backEnd.hdrMaxLuminance);

			GL_SelectTexture(0);
			GL_Bind(tr.downScaleFBOImage_quarter);
		}
		else
		{
			GL_SelectTexture(0);
			//GL_Bind(tr.downScaleFBOImage_quarter);
			GL_Bind(tr.currentRenderImage);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth,
												 tr.currentRenderImage->uploadHeight);
		}

		R_BindFBO(tr.contrastRenderFBO);
		GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		qglClear(GL_COLOR_BUFFER_BIT);

		// draw viewport
		Tess_InstantQuad(backEnd.viewParms.viewportVerts);


		// render bloom in multiple passes
#if 0
		GL_BindProgram(&tr.bloomShader);

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.bloomShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
		qglUniform1fARB(tr.bloomShader.u_BlurMagnitude, r_bloomBlur->value);
#endif
		for(i = 0; i < 2; i++)
		{
			for(j = 0; j < r_bloomPasses->integer; j++)
			{
				R_BindFBO(tr.bloomRenderFBO[(j + 1) % 2]);

				GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				qglClear(GL_COLOR_BUFFER_BIT);

				GL_State(GLS_DEPTHTEST_DISABLE);

				GL_SelectTexture(0);
				if(j == 0)
					GL_Bind(tr.contrastRenderFBOImage);
				else
					GL_Bind(tr.bloomRenderFBOImage[j % 2]);

				GL_PushMatrix();
				GL_LoadModelViewMatrix(modelView);

				MatrixOrthogonalProjection(ortho, 0, tr.bloomRenderFBO[0]->width, 0, tr.bloomRenderFBO[0]->height, -99999, 99999);
				GL_LoadProjectionMatrix(ortho);

				if(i == 0)
				{
					GL_BindProgram(&tr.blurXShader);

					qglUniform1fARB(tr.blurXShader.u_BlurMagnitude, r_bloomBlur->value);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.blurXShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
				}
				else
				{
					GL_BindProgram(&tr.blurYShader);

					qglUniform1fARB(tr.blurYShader.u_BlurMagnitude, r_bloomBlur->value);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.blurYShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
				}

				GL_PopMatrix();

				Tess_InstantQuad(backEnd.viewParms.viewportVerts);
			}

			// add offscreen processed bloom to screen
			if(DS_STANDARD_ENABLED())
			{
				R_BindFBO(tr.deferredRenderFBO);

				GL_BindProgram(&tr.screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

				GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

				GL_SelectTexture(0);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
			}
			else if(DS_PREPASS_LIGHTING_ENABLED())
			{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
				R_BindFBO(tr.deferredRenderFBO);
#else
				R_BindNullFBO();
#endif
				GL_BindProgram(&tr.screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

				GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

				GL_SelectTexture(0);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
			}
			else if(HDR_ENABLED())
			{
				R_BindFBO(tr.deferredRenderFBO);

				GL_BindProgram(&tr.screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

				GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

				GL_SelectTexture(0);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
				//GL_Bind(tr.contrastRenderFBOImage);
			}
			else
			{
				R_BindNullFBO();

				GL_BindProgram(&tr.screenShader);
				GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

				GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

				GL_SelectTexture(0);
				GL_Bind(tr.bloomRenderFBOImage[j % 2]);
				//GL_Bind(tr.contrastRenderFBOImage);
			}

			Tess_InstantQuad(backEnd.viewParms.viewportVerts);
		}
	}

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

void RB_RenderRotoscope(void)
{
	matrix_t        ortho;

	GLimp_LogComment("--- RB_CameraPostFX ---\n");

	if((backEnd.refdef.rdflags & RDF_NOWORLDMODEL) || !r_rotoscope->integer || backEnd.viewParms.isPortal)
		return;

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	// enable shader, set arrays
	GL_BindProgram(&tr.rotoscopeShader);

	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.rotoscopeShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	qglUniform1fARB(tr.rotoscopeShader.u_BlurMagnitude, r_bloomBlur->value);

	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);

	// draw viewport
	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

void RB_CameraPostFX(void)
{
	matrix_t        ortho;

	GLimp_LogComment("--- RB_CameraPostFX ---\n");

	if((backEnd.refdef.rdflags & RDF_NOWORLDMODEL) || !r_cameraPostFX->integer || backEnd.viewParms.isPortal)
		return;

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	// enable shader, set arrays
	GL_BindProgram(&tr.cameraEffectsShader);

	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.cameraEffectsShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	//qglUniform1fARB(tr.cameraEffectsShader.u_BlurMagnitude, r_bloomBlur->value);

	// bind u_CurrentMap
	GL_SelectTexture(0);
	GL_Bind(tr.occlusionRenderFBOImage);
	/*
	if(glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
		// copy depth of the main context to deferredRenderFBO
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer);
		qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
							   0, 0, glConfig.vidWidth, glConfig.vidHeight,
							   GL_COLOR_BUFFER_BIT,
							   GL_NEAREST);
	}
	else
	*/
	{
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.occlusionRenderFBOImage->uploadWidth, tr.occlusionRenderFBOImage->uploadHeight);
	}

	// bind u_GrainMap
	GL_SelectTexture(1);
	GL_Bind(tr.grainImage);

	// bind u_VignetteMap
	GL_SelectTexture(2);
	GL_Bind(tr.vignetteImage);

	// draw viewport
	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

static void RB_CalculateAdaptation()
{
	int				i;
	static float	image[64 * 64 * 4];
	float           curTime;
	float			deltaTime;
	float           luminance;
	float			avgLuminance;
	float			maxLuminance;
	double			sum;
	const vec3_t    LUMINANCE_VECTOR = {0.2125f, 0.7154f, 0.0721f};
	vec4_t			color;
	float			newAdaptation;
	float			newMaximum;

	curTime = ri.Milliseconds() / 1000.0f;

	// calculate the average scene luminance
	R_BindFBO(tr.downScaleFBO_64x64);

	// read back the contents
//	qglFinish();
	qglReadPixels(0, 0, 64, 64, GL_RGBA, GL_FLOAT, image);

	sum = 0.0f;
	maxLuminance = 0.0f;
	for(i = 0; i < (64 * 64 * 4); i += 4)
	{
		color[0] = image[i + 0];
		color[1] = image[i + 1];
		color[2] = image[i + 2];
		color[3] = image[i + 3];

		luminance = DotProduct(color, LUMINANCE_VECTOR) + 0.0001f;
		if(luminance > maxLuminance)
			maxLuminance = luminance;

		sum += log(luminance);
	}
	sum /= (64.0f * 64.0f);
	avgLuminance = exp(sum);

	// the user's adapted luminance level is simulated by closing the gap between
	// adapted luminance and current luminance by 2% every frame, based on a
	// 30 fps rate. This is not an accurate model of human adaptation, which can
	// take longer than half an hour.
	if(backEnd.hdrTime > curTime)
		backEnd.hdrTime = curTime;

	deltaTime = curTime - backEnd.hdrTime;

	//if(r_hdrMaxLuminance->value)
	{
		Q_clamp(backEnd.hdrAverageLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
		Q_clamp(avgLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);

		Q_clamp(backEnd.hdrMaxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
		Q_clamp(maxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value);
	}

	newAdaptation = backEnd.hdrAverageLuminance + (avgLuminance - backEnd.hdrAverageLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));
	newMaximum = backEnd.hdrMaxLuminance + (maxLuminance - backEnd.hdrMaxLuminance) * (1.0f - powf(0.98f, 30.0f * deltaTime));

	if(!Q_isnan(newAdaptation) && !Q_isnan(newMaximum))
	{
		#if 1
		backEnd.hdrAverageLuminance = newAdaptation;
		backEnd.hdrMaxLuminance = newMaximum;
		#else
		backEnd.hdrAverageLuminance = avgLuminance;
		backEnd.hdrMaxLuminance = maxLuminance;
		#endif
	}

	backEnd.hdrTime = curTime;

	//ri.Printf(PRINT_ALL, "RB_CalculateAdaptation: avg = %f  max = %f\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance);

	GL_CheckErrors();
}

void RB_RenderDeferredShadingResultToFrameBuffer()
{
	matrix_t        ortho;

	GLimp_LogComment("--- RB_RenderDeferredShadingResultToFrameBuffer ---\n");

	R_BindNullFBO();

	/*
	   if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	   {
	   GL_State(GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	   }
	   else
	 */
	{
		GL_State(GLS_DEPTHTEST_DISABLE);	// | GLS_DEPTHMASK_TRUE);
	}

	GL_Cull(CT_TWO_SIDED);

	// set uniforms

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);

	if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && r_hdrRendering->integer)
	{
		R_BindNullFBO();

		GL_BindProgram(&tr.toneMappingShader);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.deferredRenderFBOImage);

		if(r_hdrKey->value <= 0)
		{
			float			key;

			// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
			key = 1.03 - 2.0 / (2.0 + log10f(backEnd.hdrAverageLuminance + 1.0f));
			qglUniform1fARB(tr.toneMappingShader.u_HDRKey, key);
		}
		else
		{
			qglUniform1fARB(tr.toneMappingShader.u_HDRKey, r_hdrKey->value);
		}

		qglUniform1fARB(tr.toneMappingShader.u_HDRAverageLuminance, backEnd.hdrAverageLuminance);
		qglUniform1fARB(tr.toneMappingShader.u_HDRMaxLuminance, backEnd.hdrMaxLuminance);

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.toneMappingShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	}
	else
	{
		GL_BindProgram(&tr.screenShader);
		qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

		// bind u_ColorMap
		GL_SelectTexture(0);

		if(r_showDeferredDiffuse->integer)
		{
			GL_Bind(tr.deferredDiffuseFBOImage);
		}
		else if(r_showDeferredNormal->integer)
		{
			GL_Bind(tr.deferredNormalFBOImage);
		}
		else if(r_showDeferredSpecular->integer)
		{
			GL_Bind(tr.deferredSpecularFBOImage);
		}
		else if(r_showDeferredPosition->integer)
		{
			GL_Bind(tr.depthRenderImage);
		}
		else if(r_deferredShading->integer == DS_PREPASS_LIGHTING && r_showDeferredLight->integer)
		{
			GL_Bind(tr.lightRenderFBOImage);
		}
		else
		{
			GL_Bind(tr.deferredRenderFBOImage);
		}

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	}

	GL_CheckErrors();

	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	GL_PopMatrix();
}

void RB_RenderDeferredHDRResultToFrameBuffer()
{
	matrix_t        ortho;

	GLimp_LogComment("--- RB_RenderDeferredHDRResultToFrameBuffer ---\n");

	if(!r_hdrRendering->integer || !glConfig.framebufferObjectAvailable || !glConfig.textureFloatAvailable)
		return;

	GL_CheckErrors();

	R_BindNullFBO();

	// bind u_ColorMap
	GL_SelectTexture(0);
	GL_Bind(tr.deferredRenderFBOImage);

	GL_State(GLS_DEPTHTEST_DISABLE);
	GL_Cull(CT_TWO_SIDED);

	GL_CheckErrors();

	// set uniforms

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
									backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
									backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
									-99999, 99999);
	GL_LoadProjectionMatrix(ortho);
	GL_LoadModelViewMatrix(matrixIdentity);


	if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		GL_BindProgram(&tr.screenShader);

		qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorWhite);

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	}
	else
	{
		GL_BindProgram(&tr.toneMappingShader);

		if(r_hdrKey->value <= 0)
		{
			float			key;

			// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
			key = 1.03 - 2.0 / (2.0 + log10f(backEnd.hdrAverageLuminance + 1.0f));
			qglUniform1fARB(tr.toneMappingShader.u_HDRKey, key);
		}
		else
		{
			qglUniform1fARB(tr.toneMappingShader.u_HDRKey, r_hdrKey->value);
		}

		qglUniform1fARB(tr.toneMappingShader.u_HDRAverageLuminance, backEnd.hdrAverageLuminance);
		qglUniform1fARB(tr.toneMappingShader.u_HDRMaxLuminance, backEnd.hdrMaxLuminance);

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.toneMappingShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
	}

	GL_CheckErrors();

	Tess_InstantQuad(backEnd.viewParms.viewportVerts);

	GL_PopMatrix();
}






static void RenderLightOcclusionVolume( trRefLight_t * light)
{
	int				j;
	vec4_t          quadVerts[4];

	GL_CheckErrors();

#if 1
	if(light->isStatic && light->frustumVBO && light->frustumIBO)
	{
		// render in world space
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

		R_BindVBO(light->frustumVBO);
		R_BindIBO(light->frustumIBO);

		GL_VertexAttribsState(ATTR_POSITION);

		tess.numVertexes = light->frustumVerts;
		tess.numIndexes = light->frustumIndexes;

		Tess_DrawElements();
	}
	else
#endif
	{
		// render in light space
		R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

		tess.numIndexes = 0;
		tess.numVertexes = 0;

		switch (light->l.rlType)
		{
			case RL_OMNI:
			{
				Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], colorWhite);

				Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
				Tess_DrawElements();
				break;
			}

			case RL_PROJ:
			{
				vec3_t          farCorners[4];
				vec4_t         *frustum = light->localFrustum;

				PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
				PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

				tess.numVertexes = 0;
				tess.numIndexes = 0;

				if(!VectorCompare(light->l.projStart, vec3_origin))
				{
					vec3_t          nearCorners[4];

					// calculate the vertices defining the top area
					PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
					PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
					PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
					PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

					// draw outer surfaces
					for(j = 0; j < 4; j++)
					{
						VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
						VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
						VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
						VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
						Tess_AddQuadStamp2(quadVerts, colorCyan);
					}

					// draw far cap
					VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
					VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
					VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
					VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorRed);

					// draw near cap
					VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
					VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
					VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
					VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorGreen);

				}
				else
				{
					vec3_t	top;

					// no light_start, just use the top vertex (doesn't need to be mirrored)
					PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

					// draw pyramid
					for(j = 0; j < 4; j++)
					{
						VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
						VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
						tess.indexes[tess.numIndexes++] = tess.numVertexes;
						tess.numVertexes++;

						VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
						VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
						tess.indexes[tess.numIndexes++] = tess.numVertexes;
						tess.numVertexes++;

						VectorCopy(top, tess.xyz[tess.numVertexes]);
						VectorCopy4(colorCyan, tess.colors[tess.numVertexes]);
						tess.indexes[tess.numIndexes++] = tess.numVertexes;
						tess.numVertexes++;
					}

					VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
					VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
					VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
					VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
					Tess_AddQuadStamp2(quadVerts, colorRed);
				}

				Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
				Tess_DrawElements();
				break;
			}

			default:
				break;
		}
	}

	tess.numIndexes = 0;
	tess.numVertexes = 0;

	GL_CheckErrors();
}

static void IssueLightOcclusionQuery(link_t * queue, trRefLight_t * light, qboolean resetMultiQueryLink)
{
	GLimp_LogComment("--- IssueLightOcclusionQuery ---\n");

	//ri.Printf(PRINT_ALL, "--- IssueOcclusionQuery(%i) ---\n", node - tr.world->nodes);

	if(tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES -1))
	{
		light->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		light->occlusionQueryObject = 0;
	}

	EnQueue(queue, light);

	// tell GetOcclusionQueryResult that this is not a multi query
	if(resetMultiQueryLink)
	{
		QueueInit(&light->multiQuery);
	}

	if(light->occlusionQueryObject > 0)
	{
		GL_CheckErrors();

		// begin the occlusion query
		qglBeginQueryARB(GL_SAMPLES_PASSED, light->occlusionQueryObject);

		GL_CheckErrors();

		RenderLightOcclusionVolume(light);

		// end the query
		qglEndQueryARB(GL_SAMPLES_PASSED);

#if 1
		if(!qglIsQueryARB(light->occlusionQueryObject))
		{
			ri.Error(ERR_FATAL, "IssueLightOcclusionQuery: light %i has no occlusion query object in slot %i: %i", light - tr.world->lights, backEnd.viewParms.viewCount, light->occlusionQueryObject);
		}
#endif

		//light->occlusionQueryNumbers[backEnd.viewParms.viewCount] = backEnd.pc.c_occlusionQueries;
		backEnd.pc.c_occlusionQueries++;
	}

	GL_CheckErrors();
}

static void IssueLightMultiOcclusionQueries(link_t * multiQueue, link_t * individualQueue)
{
	trRefLight_t *light;
	trRefLight_t *multiQueryLight;
	link_t *l;

	GLimp_LogComment("--- IssueLightMultiOcclusionQueries ---\n");

#if 0
	ri.Printf(PRINT_ALL, "IssueLightMultiOcclusionQueries(");
	for(l = multiQueue->prev; l != multiQueue; l = l->prev)
	{
		light = (trRefLight_t *) l->data;

		ri.Printf(PRINT_ALL, "%i, ", light - backEnd.refdef.lights);
	}
	ri.Printf(PRINT_ALL, ")\n");
#endif

	if(QueueEmpty(multiQueue))
		return;

	multiQueryLight = (trRefLight_t *) QueueFront(multiQueue)->data;

	if(tr.numUsedOcclusionQueryObjects < (MAX_OCCLUSION_QUERIES -1))
	{
		multiQueryLight->occlusionQueryObject = tr.occlusionQueryObjects[tr.numUsedOcclusionQueryObjects++];
	}
	else
	{
		multiQueryLight->occlusionQueryObject = 0;
	}

	if(multiQueryLight->occlusionQueryObject > 0)
	{
		// begin the occlusion query
		GL_CheckErrors();

		qglBeginQueryARB(GL_SAMPLES_PASSED, multiQueryLight->occlusionQueryObject);

		GL_CheckErrors();

		//ri.Printf(PRINT_ALL, "rendering nodes:[");
		for(l = multiQueue->prev; l != multiQueue; l = l->prev)
		{
			light = (trRefLight_t *) l->data;

			//ri.Printf(PRINT_ALL, "%i, ", light - backEnd.refdef.lights);

			RenderLightOcclusionVolume(light);
		}
		//ri.Printf(PRINT_ALL, "]\n");

		backEnd.pc.c_occlusionQueries++;
		backEnd.pc.c_occlusionQueriesMulti++;

		// end the query
		qglEndQueryARB(GL_SAMPLES_PASSED);

		GL_CheckErrors();

#if 0
		if(!qglIsQueryARB(multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			ri.Error(ERR_FATAL, "IssueMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, backEnd.viewParms.viewCount, multiQueryNode->occlusionQueryObjects[backEnd.viewParms.viewCount]);
		}
#endif
	}

	// move queue to node->multiQuery queue
	QueueInit(&multiQueryLight->multiQuery);
	DeQueue(multiQueue);
	while(!QueueEmpty(multiQueue))
	{
		light = (trRefLight_t *) DeQueue(multiQueue);
		EnQueue(&multiQueryLight->multiQuery, light);
	}

	EnQueue(individualQueue, multiQueryLight);

	//ri.Printf(PRINT_ALL, "--- IssueMultiOcclusionQueries end ---\n");
}

static qboolean LightOcclusionResultAvailable(trRefLight_t *light)
{
	GLint			available;

	if(light->occlusionQueryObject > 0)
	{
		qglFinish();

		available = 0;
		//if(qglIsQueryARB(light->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			qglGetQueryObjectivARB(light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
			GL_CheckErrors();
		}

		return !!available;
	}

	return qtrue;
}

static void GetLightOcclusionQueryResult(trRefLight_t *light)
{
	link_t			*l, *sentinel;
	int			     ocSamples;
	GLint			 available;

	GLimp_LogComment("--- GetLightOcclusionQueryResult ---\n");

	if(light->occlusionQueryObject > 0)
	{
		qglFinish();

#if 0
		if(!qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			ri.Error(ERR_FATAL, "GetOcclusionQueryResult: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, backEnd.viewParms.viewCount, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);
		}
#endif

		available = 0;
		while(!available)
		{
			//if(qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				qglGetQueryObjectivARB(light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
				//GL_CheckErrors();
			}
		}

		backEnd.pc.c_occlusionQueriesAvailable++;

		qglGetQueryObjectivARB(light->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples);

		//ri.Printf(PRINT_ALL, "GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

		GL_CheckErrors();
	}
	else
	{
		ocSamples = 1;
	}

	light->occlusionQuerySamples = ocSamples;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &light->multiQuery;
	for(l = sentinel->prev; l != sentinel; l = l->prev)
	{
		light = (trRefLight_t *) l->data;

		light->occlusionQuerySamples = ocSamples;
	}
}

static int LightCompare(const void *a, const void *b)
{
	trRefLight_t   *l1, *l2;
	float           d1, d2;

	l1 = (trRefLight_t *) *(void **)a;
	l2 = (trRefLight_t *) *(void **)b;

	d1 = DistanceSquared(backEnd.viewParms.orientation.origin, l1->l.origin);
	d2 = DistanceSquared(backEnd.viewParms.orientation.origin, l2->l.origin);

	if(d1 < d2)
	{
		return -1;
	}
	if(d1 > d2)
	{
		return 1;
	}

	return 0;
}

void RB_RenderLightOcclusionQueries()
{
	GLimp_LogComment("--- RB_RenderLightOcclusionQueries ---\n");

	if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		int				i;
		interaction_t  *ia;
		int             iaCount;
		int             iaFirst;
		trRefLight_t   *light, *oldLight, *multiQueryLight;
		GLint           ocSamples = 0;
		qboolean        queryObjects;
		link_t			occlusionQueryQueue;
		link_t			invisibleQueue;
		growList_t      invisibleList;
		int             startTime = 0, endTime = 0;

		qglVertexAttrib4fARB(ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f);

		if(r_speeds->integer == 7)
		{
			qglFinish();
			startTime = ri.Milliseconds();
		}

		GL_BindProgram(&tr.genericSingleShader);
		GL_Cull(CT_TWO_SIDED);

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		// don't write to the color buffer or depth buffer
		if(r_showOcclusionQueries->integer)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		}
		else
		{
			GL_State(GLS_COLORMASK_BITS);
		}

		tr.numUsedOcclusionQueryObjects = 0;
		QueueInit(&occlusionQueryQueue);
		QueueInit(&invisibleQueue);
		Com_InitGrowList(&invisibleList, 1000);

		// loop trough all light interactions and render the light OBB for each last interaction
		for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
		{
			backEnd.currentLight = light = ia->light;
			ia->occlusionQuerySamples = 1;

			if(!ia->next)
			{
				// last interaction of current light
				if(!ia->noOcclusionQueries)
				{
					Com_AddToGrowList(&invisibleList, light);
				}

				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// sort lights by distance
		qsort(invisibleList.elements, invisibleList.currentElements, sizeof(void *), LightCompare);

		for(i = 0; i < invisibleList.currentElements; i++)
		{
			light = Com_GrowListElement(&invisibleList, i);

			EnQueue(&invisibleQueue, light);

			if((invisibleList.currentElements - i) <= 100)
			{
				if(QueueSize(&invisibleQueue) >= 10)
					IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
			}
			else
			{
				if(QueueSize(&invisibleQueue) >= 50)
					IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
			}
		}
		Com_DestroyGrowList(&invisibleList);

		if(!QueueEmpty(&invisibleQueue))
		{
			// remaining previously invisible node queries
			IssueLightMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);

			//ri.Printf(PRINT_ALL, "occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		while(!QueueEmpty(&occlusionQueryQueue))
		{
			if(LightOcclusionResultAvailable(QueueFront(&occlusionQueryQueue)->data))
			{
				light = (trRefLight_t *) DeQueue(&occlusionQueryQueue);

				// wait if result not available
				GetLightOcclusionQueryResult(light);

				if(light->occlusionQuerySamples > r_chcVisibilityThreshold->integer)
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if(!QueueEmpty(&light->multiQuery))
					{
						multiQueryLight = light;

						IssueLightOcclusionQuery(&occlusionQueryQueue, multiQueryLight, qfalse);

						while(!QueueEmpty(&multiQueryLight->multiQuery))
						{
							light = (trRefLight_t *) DeQueue(&multiQueryLight->multiQuery);

							IssueLightOcclusionQuery(&occlusionQueryQueue, light, qtrue);
						}
					}
				}
				else
				{
					if(!QueueEmpty(&light->multiQuery))
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;

						multiQueryLight = light;
						while(!QueueEmpty(&multiQueryLight->multiQuery))
						{
							light = (trRefLight_t *) DeQueue(&multiQueryLight->multiQuery);

							backEnd.pc.c_occlusionQueriesLightsCulled++;
							backEnd.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;
					}
				}
			}
		}

		if(r_speeds->integer == 7)
		{
			qglFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_occlusionQueriesResponseTime = endTime - startTime;

			startTime = ri.Milliseconds();
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

		// reenable writes to depth and color buffers
		GL_State(GLS_DEPTHMASK_TRUE);

		// loop trough all light interactions and fetch results for each last interaction
		// then copy result to all other interactions that belong to the same light
		iaFirst = 0;
		queryObjects = qtrue;
		oldLight = NULL;
		for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
		{
			backEnd.currentLight = light = ia->light;

			if(light != oldLight)
			{
				iaFirst = iaCount;
			}

			if(!queryObjects)
			{
				ia->occlusionQuerySamples = ocSamples;

				if(ocSamples <= 0)
				{
					backEnd.pc.c_occlusionQueriesInteractionsCulled++;
				}
			}

			if(!ia->next)
			{
				if(queryObjects)
				{
					if(!ia->noOcclusionQueries)
					{
						ocSamples = light->occlusionQuerySamples > r_chcVisibilityThreshold->integer;
					}
					else
					{
						ocSamples = 1;
					}

					// jump back to first interaction of this light copy query result
					ia = &backEnd.viewParms.interactions[iaFirst];
					iaCount = iaFirst;
					queryObjects = qfalse;
				}
				else
				{
					if(iaCount < (backEnd.viewParms.numInteractions - 1))
					{
						// jump to next interaction and start querying
						ia++;
						iaCount++;
						queryObjects = qtrue;
					}
					else
					{
						// increase last time to leave for loop
						iaCount++;
					}
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}

			oldLight = light;
		}

		if(r_speeds->integer == 7)
		{
			qglFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_occlusionQueriesFetchTime = endTime - startTime;
		}
	}

	GL_CheckErrors();
}

#if 0
void RB_RenderBspOcclusionQueries()
{
	GLimp_LogComment("--- RB_RenderBspOcclusionQueries ---\n");

	if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer)
	{
		//int             j;
		bspNode_t      *node;
		link_t		   *l, *next, *sentinel;

		GL_BindProgram(&tr.genericSingleShader);
		GL_Cull(CT_TWO_SIDED);

		GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		// don't write to the color buffer or depth buffer
		GL_State(GLS_COLORMASK_BITS);

		sentinel = &tr.occlusionQueryList;
		for(l = sentinel->next; l != sentinel; l = next)
		{
			next = l->next;
			node = (bspNode_t *) l->data;

			// begin the occlusion query
			qglBeginQueryARB(GL_SAMPLES_PASSED, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);

			R_BindVBO(node->volumeVBO);
			R_BindIBO(node->volumeIBO);

			GL_VertexAttribsState(ATTR_POSITION);

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes = node->volumeIndexes;

			Tess_DrawElements();

			// end the query
			// don't read back immediately so that we give the query time to be ready
			qglEndQueryARB(GL_SAMPLES_PASSED);

#if 0
			if(!qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				ri.Error(ERR_FATAL, "node %i has no occlusion query object in slot %i: %i", j, 0, node->occlusionQueryObjects[backEnd.viewParms.viewCount]);
			}
#endif

			backEnd.pc.c_occlusionQueries++;

			tess.numIndexes = 0;
			tess.numVertexes = 0;
		}
	}

	GL_CheckErrors();
}


void RB_CollectBspOcclusionQueries()
{
	GLimp_LogComment("--- RB_CollectBspOcclusionQueries ---\n");

	if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer)
	{
		//int             j;
		bspNode_t      *node;
		link_t		   *l, *next, *sentinel;

		int				ocCount;
		int             avCount;
		GLint           available;

		qglFinish();

		ocCount = 0;
		sentinel = &tr.occlusionQueryList;
		for(l = sentinel->next; l != sentinel; l = l->next)
		{
			node = (bspNode_t *) l->data;

			if(qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				ocCount++;
			}
		}

		//ri.Printf(PRINT_ALL, "waiting for %i queries...\n", ocCount);

		avCount = 0;
		do
		{
			for(l = sentinel->next; l != sentinel; l = l->next)
			{
				node = (bspNode_t *) l->data;

				if(node->issueOcclusionQuery)
				{
					available = 0;
					if(qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
					{
						qglGetQueryObjectivARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
						GL_CheckErrors();
					}

					if(available)
					{
						node->issueOcclusionQuery = qfalse;
						avCount++;

						//if(//avCount % oc)

						//ri.Printf(PRINT_ALL, "%i queries...\n", avCount);
					}
				}
			}

		} while(avCount < ocCount);

		for(l = sentinel->next; l != sentinel; l = l->next)
		{
			node = (bspNode_t *) l->data;

			available = 0;
			if(qglIsQueryARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				qglGetQueryObjectivARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
				GL_CheckErrors();
			}

			if(available)
			{
				backEnd.pc.c_occlusionQueriesAvailable++;

				// get the object and store it in the occlusion bits for the light
				qglGetQueryObjectivARB(node->occlusionQueryObjects[backEnd.viewParms.viewCount], GL_QUERY_RESULT, &node->occlusionQuerySamples[backEnd.viewParms.viewCount]);

				if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] <= 0)
				{
					backEnd.pc.c_occlusionQueriesLeafsCulled++;
				}
			}
			else
			{
				node->occlusionQuerySamples[backEnd.viewParms.viewCount] = 1;
			}

			GL_CheckErrors();
		}

		//ri.Printf(PRINT_ALL, "done\n");
	}
}
#endif

static void RB_RenderDebugUtils()
{
	GLimp_LogComment("--- RB_RenderDebugUtils ---\n");

	if(r_showLightTransforms->integer || r_showShadowLod->integer)
	{
		interaction_t  *ia;
		int             iaCount, j;
		trRefLight_t   *light;
		vec3_t          forward, left, up;
		vec4_t          lightColor;
		vec4_t          quadVerts[4];

		vec3_t			minSize = {-2, -2, -2};
		vec3_t			maxSize = { 2,  2,  2};

		GL_BindProgram(&tr.genericSingleShader);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_CUSTOM_RGB);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_CUSTOM);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
		{
			light = ia->light;

			if(!ia->next)
			{
				if(r_showShadowLod->integer)
				{
					if(light->shadowLOD == 0)
					{
						VectorCopy4(colorRed, lightColor);
					}
					else if(light->shadowLOD == 1)
					{
						VectorCopy4(colorGreen, lightColor);
					}
					else if(light->shadowLOD == 2)
					{
						VectorCopy4(colorBlue, lightColor);
					}
					else if(light->shadowLOD == 3)
					{
						VectorCopy4(colorYellow, lightColor);
					}
					else if(light->shadowLOD == 4)
					{
						VectorCopy4(colorMagenta, lightColor);
					}
					else if(light->shadowLOD == 5)
					{
						VectorCopy4(colorCyan, lightColor);
					}
					else
					{
						VectorCopy4(colorMdGrey, lightColor);
					}
				}
				else// if(r_deferredShading->integer == DS_PREPASS_LIGHTING)
				{
					if(!ia->occlusionQuerySamples)
					{
						VectorCopy4(colorRed, lightColor);
					}
					else
					{
						VectorCopy4(colorGreen, lightColor);
					}
				}
				/*
				else
				{
					VectorCopy4(g_color_table[iaCount % 8], lightColor);
				}
				*/

				GLSL_SetUniform_Color(&tr.genericSingleShader, lightColor);

				MatrixToVectorsFLU(matrixIdentity, forward, left, up);
				VectorMA(vec3_origin, 16, forward, forward);
				VectorMA(vec3_origin, 16, left, left);
				VectorMA(vec3_origin, 16, up, up);

				/*
				// draw axis
				qglBegin(GL_LINES);

				// draw orientation
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorRed);
				qglVertex3fv(vec3_origin);
				qglVertex3fv(forward);

				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorGreen);
				qglVertex3fv(vec3_origin);
				qglVertex3fv(left);

				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorBlue);
				qglVertex3fv(vec3_origin);
				qglVertex3fv(up);

				// draw special vectors
				qglVertexAttrib4fvARB(ATTR_INDEX_COLOR, colorYellow);
				qglVertex3fv(vec3_origin);
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
				qglVertex3fv(light->transformed);

				qglEnd();
				*/

#if 1
				if(light->isStatic && light->frustumVBO && light->frustumIBO)
				{
					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					R_BindVBO(light->frustumVBO);
					R_BindIBO(light->frustumIBO);

					GL_VertexAttribsState(ATTR_POSITION);

					tess.numVertexes = light->frustumVerts;
					tess.numIndexes = light->frustumIndexes;

					Tess_DrawElements();

					tess.numIndexes = 0;
					tess.numVertexes = 0;
				}
				else
#endif
				{
					// set up the transformation matrix
					R_RotateLightForViewParms(light, &backEnd.viewParms, &backEnd.orientation);
					GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					tess.numIndexes = 0;
					tess.numVertexes = 0;

					switch (light->l.rlType)
					{
						case RL_OMNI:
						case RL_DIRECTIONAL:
						{
							Tess_AddCube(vec3_origin, light->localBounds[0], light->localBounds[1], lightColor);

							if(!VectorCompare(light->l.center, vec3_origin))
								Tess_AddCube(light->l.center, minSize, maxSize, colorYellow);

							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							Tess_DrawElements();
							break;
						}

						case RL_PROJ:
						{
							vec3_t          farCorners[4];
							//vec4_t			frustum[6];
							vec4_t         *frustum = light->localFrustum;

#if 0
							// transform frustum from world space to local space
							for(j = 0; j < 6; j++)
							{
								MatrixTransformPlane(light->transformMatrix, light->localFrustum[j], frustum[j]);
								//VectorCopy4(light->localFrustum[j], frustum[j]);
								//MatrixTransformPlane2(light->viewMatrix, frustum[j]);
							}

							// go back to the world modelview matrix
							backEnd.orientation = backEnd.viewParms.world;
							GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
							GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);
#endif

							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[0]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_FAR], farCorners[1]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[2]);
							PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_FAR], farCorners[3]);

							// the planes of the frustum are measured at world 0,0,0 so we have to position the intersection points relative to the light origin
	#if 0
							ri.Printf(PRINT_ALL, "pyramid farCorners\n");
							for(j = 0; j < 4; j++)
							{
								ri.Printf(PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", farCorners[j][0], farCorners[j][1], farCorners[j][2]);
							}
	#endif

							tess.numVertexes = 0;
							tess.numIndexes = 0;


							if(!VectorCompare(light->l.projStart, vec3_origin))
							{
								vec3_t          nearCorners[4];

								// calculate the vertices defining the top area
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[0]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], frustum[FRUSTUM_NEAR], nearCorners[1]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[2]);
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_BOTTOM], frustum[FRUSTUM_NEAR], nearCorners[3]);

	#if 0
								ri.Printf(PRINT_ALL, "pyramid nearCorners\n");
								for(j = 0; j < 4; j++)
								{
									ri.Printf(PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", nearCorners[j][0], nearCorners[j][1], nearCorners[j][2]);
								}
	#endif

								// draw outer surfaces
								for(j = 0; j < 4; j++)
								{
									VectorSet4(quadVerts[0], nearCorners[j][0], nearCorners[j][1], nearCorners[j][2], 1);
									VectorSet4(quadVerts[1], farCorners[j][0], farCorners[j][1], farCorners[j][2], 1);
									VectorSet4(quadVerts[2], farCorners[(j + 1) % 4][0], farCorners[(j + 1) % 4][1], farCorners[(j + 1) % 4][2], 1);
									VectorSet4(quadVerts[3], nearCorners[(j + 1) % 4][0], nearCorners[(j + 1) % 4][1], nearCorners[(j + 1) % 4][2], 1);
									Tess_AddQuadStamp2(quadVerts, lightColor);
								}

								// draw far cap
								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, lightColor);

								// draw near cap
								VectorSet4(quadVerts[0], nearCorners[0][0], nearCorners[0][1], nearCorners[0][2], 1);
								VectorSet4(quadVerts[1], nearCorners[1][0], nearCorners[1][1], nearCorners[1][2], 1);
								VectorSet4(quadVerts[2], nearCorners[2][0], nearCorners[2][1], nearCorners[2][2], 1);
								VectorSet4(quadVerts[3], nearCorners[3][0], nearCorners[3][1], nearCorners[3][2], 1);
								Tess_AddQuadStamp2(quadVerts, lightColor);

							}
							else
							{
								vec3_t	top;

								// no light_start, just use the top vertex (doesn't need to be mirrored)
								PlanesGetIntersectionPoint(frustum[FRUSTUM_LEFT], frustum[FRUSTUM_RIGHT], frustum[FRUSTUM_TOP], top);

								// draw pyramid
								for(j = 0; j < 4; j++)
								{
									VectorCopy(farCorners[j], tess.xyz[tess.numVertexes]);
									VectorCopy4(lightColor, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(farCorners[(j + 1) % 4], tess.xyz[tess.numVertexes]);
									VectorCopy4(lightColor, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;

									VectorCopy(top, tess.xyz[tess.numVertexes]);
									VectorCopy4(lightColor, tess.colors[tess.numVertexes]);
									tess.indexes[tess.numIndexes++] = tess.numVertexes;
									tess.numVertexes++;
								}

								VectorSet4(quadVerts[0], farCorners[3][0], farCorners[3][1], farCorners[3][2], 1);
								VectorSet4(quadVerts[1], farCorners[2][0], farCorners[2][1], farCorners[2][2], 1);
								VectorSet4(quadVerts[2], farCorners[1][0], farCorners[1][1], farCorners[1][2], 1);
								VectorSet4(quadVerts[3], farCorners[0][0], farCorners[0][1], farCorners[0][2], 1);
								Tess_AddQuadStamp2(quadVerts, lightColor);
							}

							// draw light_target
							Tess_AddCube(light->l.projTarget, minSize, maxSize, colorRed);
							Tess_AddCube(light->l.projRight, minSize, maxSize, colorGreen);
							Tess_AddCube(light->l.projUp, minSize, maxSize, colorBlue);

							if(!VectorCompare(light->l.projStart, vec3_origin))
								Tess_AddCube(light->l.projStart, minSize, maxSize, colorYellow);

							if(!VectorCompare(light->l.projEnd, vec3_origin))
								Tess_AddCube(light->l.projEnd, minSize, maxSize, colorMagenta);


							Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
							Tess_DrawElements();
							break;
						}

						default:
							break;
					}

					tess.numIndexes = 0;
					tess.numVertexes = 0;
				}

				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if(r_showLightInteractions->integer)
	{
		int             i;
		int             cubeSides;
		interaction_t  *ia;
		int             iaCount;
		trRefLight_t   *light;
		trRefEntity_t  *entity;
		surfaceType_t  *surface;
		vec4_t          lightColor;

		GL_BindProgram(&tr.genericSingleShader);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
		{
			backEnd.currentEntity = entity = ia->entity;
			light = ia->light;
			surface = ia->surface;

			if(entity != &tr.worldEntity)
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation);
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

			if(r_shadows->integer >= 4 && light->l.rlType == RL_OMNI)
			{
#if 0
				VectorCopy4(colorMdGrey, lightColor);

				if(ia->cubeSideBits & CUBESIDE_PX)
				{
					VectorCopy4(colorBlack, lightColor);
				}
				if(ia->cubeSideBits & CUBESIDE_PY)
				{
					VectorCopy4(colorRed, lightColor);
				}
				if(ia->cubeSideBits & CUBESIDE_PZ)
				{
					VectorCopy4(colorGreen, lightColor);
				}
				if(ia->cubeSideBits & CUBESIDE_NX)
				{
					VectorCopy4(colorYellow, lightColor);
				}
				if(ia->cubeSideBits & CUBESIDE_NY)
				{
					VectorCopy4(colorBlue, lightColor);
				}
				if(ia->cubeSideBits & CUBESIDE_NZ)
				{
					VectorCopy4(colorCyan, lightColor);
				}
				if(ia->cubeSideBits == CUBESIDE_CLIPALL)
				{
					VectorCopy4(colorMagenta, lightColor);
				}
#else
				// count how many cube sides are in use for this interaction
				cubeSides = 0;
				for(i = 0; i < 6; i++)
				{
					if(ia->cubeSideBits & (1 << i))
					{
						cubeSides++;
					}
				}

				VectorCopy4(g_color_table[cubeSides], lightColor);
#endif
			}
			else
			{
				VectorCopy4(colorMdGrey, lightColor);
			}

			lightColor[0] *= 0.5f;
			lightColor[1] *= 0.5f;
			lightColor[2] *= 0.5f;
			//lightColor[3] *= 0.2f;

			VectorCopy4(colorWhite, lightColor);

			tess.numVertexes = 0;
			tess.numIndexes = 0;

			if(*surface == SF_FACE)
			{
				srfSurfaceFace_t *face;

				face = (srfSurfaceFace_t *) surface;
				Tess_AddCube(vec3_origin, face->bounds[0], face->bounds[1], lightColor);
			}
			else if(*surface == SF_GRID)
			{
				srfGridMesh_t  *grid;

				grid = (srfGridMesh_t *) surface;
				Tess_AddCube(vec3_origin, grid->meshBounds[0], grid->meshBounds[1], lightColor);
			}
			else if(*surface == SF_TRIANGLES)
			{
				srfTriangles_t *tri;

				tri = (srfTriangles_t *) surface;
				Tess_AddCube(vec3_origin, tri->bounds[0], tri->bounds[1], lightColor);
			}
			else if(*surface == SF_VBO_MESH)
			{
				srfVBOMesh_t   *srf = (srfVBOMesh_t *) surface;
				Tess_AddCube(vec3_origin, srf->bounds[0], srf->bounds[1], lightColor);
			}
			else if(*surface == SF_MDX)
			{
				Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], lightColor);
			}

			Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
			Tess_DrawElements();

			tess.numIndexes = 0;
			tess.numVertexes = 0;

			if(!ia->next)
			{
				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if(r_showEntityTransforms->integer)
	{
		trRefEntity_t  *ent;
		int             i;
		vec4_t          quadVerts[4];

		GL_BindProgram(&tr.genericSingleShader);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		ent = backEnd.refdef.entities;
		for(i = 0; i < backEnd.refdef.numEntities; i++, ent++)
		{
			if((ent->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal)
				continue;

			// set up the transformation matrix
			R_RotateEntityForViewParms(ent, &backEnd.viewParms, &backEnd.orientation);
			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

			R_DebugAxis(vec3_origin, matrixIdentity);
			//R_DebugBoundingBox(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorMagenta);
			tess.numIndexes = 0;
			tess.numVertexes = 0;

			VectorSet4(quadVerts[0], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorRed);

			VectorSet4(quadVerts[0], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorGreen);

			VectorSet4(quadVerts[0], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorBlue);

			VectorSet4(quadVerts[0], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorYellow);

			VectorSet4(quadVerts[0], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[0][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[1][0], ent->localBounds[0][1], ent->localBounds[0][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorMagenta);

			VectorSet4(quadVerts[0], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			VectorSet4(quadVerts[1], ent->localBounds[1][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[2], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[1][2], 1);
			VectorSet4(quadVerts[3], ent->localBounds[0][0], ent->localBounds[1][1], ent->localBounds[0][2], 1);
			Tess_AddQuadStamp2(quadVerts, colorCyan);

			Tess_UpdateVBOs(ATTR_POSITION | ATTR_COLOR);
			Tess_DrawElements();

			tess.numIndexes = 0;
			tess.numVertexes = 0;


			// go back to the world modelview matrix
			//backEnd.orientation = backEnd.viewParms.world;
			//GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

			//R_DebugBoundingBox(vec3_origin, ent->worldBounds[0], ent->worldBounds[1], colorCyan);
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	if(r_showSkeleton->integer)
	{
		int             i, j, k, parentIndex;
		trRefEntity_t  *ent;
		vec3_t          origin, offset;
		vec3_t          forward, right, up;
		vec3_t          diff, tmp, tmp2, tmp3;
		vec_t           length;
		vec4_t          tetraVerts[4];
		static refSkeleton_t skeleton;
		refSkeleton_t  *skel;

		GL_BindProgram(&tr.genericSingleShader);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.charsetImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		ent = backEnd.refdef.entities;
		for(i = 0; i < backEnd.refdef.numEntities; i++, ent++)
		{
			if((ent->e.renderfx & RF_THIRD_PERSON) && !backEnd.viewParms.isPortal)
				continue;

			// set up the transformation matrix
			R_RotateEntityForViewParms(ent, &backEnd.viewParms, &backEnd.orientation);
			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);


			tess.numVertexes = 0;
			tess.numIndexes = 0;

			skel = NULL;
			if(ent->e.skeleton.type == SK_ABSOLUTE)
			{
				skel = &ent->e.skeleton;
			}
			else
			{
				model_t        *model;
				refBone_t      *bone;

				model = R_GetModelByHandle(ent->e.hModel);

				if(model)
				{
					switch (model->type)
					{
						case MOD_MD5:
						{
							// copy absolute bones
							skeleton.numBones = model->md5->numBones;
							for(j = 0, bone = &skeleton.bones[0]; j < skeleton.numBones; j++, bone++)
							{
								#if defined(REFBONE_NAMES)
								Q_strncpyz(bone->name, model->md5->bones[j].name, sizeof(bone->name));
								#endif

								bone->parentIndex = model->md5->bones[j].parentIndex;
								VectorCopy(model->md5->bones[j].origin, bone->origin);
								VectorCopy(model->md5->bones[j].rotation, bone->rotation);
							}

							skel = &skeleton;
							break;
						}

						default:
							break;
					}
				}
			}

			if(skel)
			{
				static vec3_t	worldOrigins[MAX_BONES];

				GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);

				for(j = 0; j < skel->numBones; j++)
				{
					parentIndex = skel->bones[j].parentIndex;

					if(parentIndex < 0)
					{
						VectorClear(origin);
					}
					else
					{
						VectorCopy(skel->bones[parentIndex].origin, origin);
					}
					VectorCopy(skel->bones[j].origin, offset);
					QuatToVectorsFRU(skel->bones[j].rotation, forward, right, up);

					VectorSubtract(offset, origin, diff);
					if((length = VectorNormalize(diff)))
					{
						PerpendicularVector(tmp, diff);
						//VectorCopy(up, tmp);

						VectorScale(tmp, length * 0.1, tmp2);
						VectorMA(tmp2, length * 0.2, diff, tmp2);

						for(k = 0; k < 3; k++)
						{
							RotatePointAroundVector(tmp3, diff, tmp2, k * 120);
							VectorAdd(tmp3, origin, tmp3);
							VectorCopy(tmp3, tetraVerts[k]);
							tetraVerts[k][3] = 1;
						}

						VectorCopy(origin, tetraVerts[3]);
						tetraVerts[3][3] = 1;
						Tess_AddTetrahedron(tetraVerts, g_color_table[ColorIndex(j)]);

						VectorCopy(offset, tetraVerts[3]);
						tetraVerts[3][3] = 1;
						Tess_AddTetrahedron(tetraVerts, g_color_table[ColorIndex(j)]);
					}

					MatrixTransformPoint(backEnd.orientation.transformMatrix, skel->bones[j].origin, worldOrigins[j]);
				}

				Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR);

				Tess_DrawElements();

				tess.numVertexes = 0;
				tess.numIndexes = 0;

#if defined(REFBONE_NAMES)
				{
					GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
					GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

					// draw names
					for(j = 0; j < skel->numBones; j++)
					{
						vec3_t          left, up;
						float           radius;
						vec3_t			origin;

						// calculate the xyz locations for the four corners
						radius = 0.4;
						VectorScale(backEnd.viewParms.orientation.axis[1], radius, left);
						VectorScale(backEnd.viewParms.orientation.axis[2], radius, up);

						if(backEnd.viewParms.isMirror)
						{
							VectorSubtract(vec3_origin, left, left);
						}

						for(k = 0; k < strlen(skel->bones[j].name); k++)
						{
							int				ch;
							int             row, col;
							float           frow, fcol;
							float           size;

							ch = skel->bones[j].name[k];
							ch &= 255;

							if(ch == ' ')
							{
								break;
							}

							row = ch >> 4;
							col = ch & 15;

							frow = row * 0.0625;
							fcol = col * 0.0625;
							size = 0.0625;

							VectorMA(worldOrigins[j], -(k + 2.0f), left, origin);
							Tess_AddQuadStampExt(origin, left, up, colorWhite, fcol, frow, fcol + size, frow + size);
						}

						Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR);

						Tess_DrawElements();

						tess.numVertexes = 0;
						tess.numIndexes = 0;
					}
				}
#endif // REFBONE_NAMES
			}

			tess.numVertexes = 0;
			tess.numIndexes = 0;
		}
	}

	if(r_showLightScissors->integer)
	{
		interaction_t  *ia;
		int             iaCount;
		matrix_t        ortho;
		vec4_t          quadVerts[4];

		GL_BindProgram(&tr.genericSingleShader);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_CUSTOM_RGB);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_CUSTOM);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		// set 2D virtual screen size
		GL_PushMatrix();
		MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
										backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
										backEnd.viewParms.viewportY,
										backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999);
		GL_LoadProjectionMatrix(ortho);
		GL_LoadModelViewMatrix(matrixIdentity);

		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

		for(iaCount = 0, ia = &backEnd.viewParms.interactions[0]; iaCount < backEnd.viewParms.numInteractions;)
		{
			if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA)
			{
				if(!ia->occlusionQuerySamples)
				{
					GLSL_SetUniform_Color(&tr.genericSingleShader, colorRed);
				}
				else
				{
					GLSL_SetUniform_Color(&tr.genericSingleShader, colorGreen);
				}

				VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Tess_InstantQuad(quadVerts);
			}
			else if(r_shadows->integer == 3 && qglDepthBoundsEXT)
			{
				if(ia->noDepthBoundsTest)
				{
					GLSL_SetUniform_Color(&tr.genericSingleShader, colorBlue);
				}
				else
				{
					GLSL_SetUniform_Color(&tr.genericSingleShader, colorGreen);
				}

				VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Tess_InstantQuad(quadVerts);
			}
			else
			{
				GLSL_SetUniform_Color(&tr.genericSingleShader, colorWhite);

				VectorSet4(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
				VectorSet4(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				VectorSet4(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
				Tess_InstantQuad(quadVerts);
			}

			if(!ia->next)
			{
				if(iaCount < (backEnd.viewParms.numInteractions - 1))
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		GL_PopMatrix();
	}

#if 1
	if(r_showCubeProbes->integer)
	{
		cubemapProbe_t *cubeProbe;
		int             j;
		vec4_t          quadVerts[4];
		vec3_t			mins = {-8, -8, -8};
		vec3_t			maxs = { 8,  8,  8};
		vec3_t			viewOrigin;

		if(tr.refdef.rdflags & (RDF_NOWORLDMODEL | RDF_NOCUBEMAP))
		{
			return;
		}

		// enable shader, set arrays
		GL_BindProgram(&tr.reflectionShader_C);
		GL_State(0);
		GL_Cull(CT_FRONT_SIDED);

		// set uniforms
		VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin);	// in world space
		GLSL_SetUniform_ViewOrigin(&tr.reflectionShader_C, viewOrigin);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.reflectionShader_C, qfalse);
		}

		for(j = 0; j < tr.cubeProbes.currentElements; j++)
		{
			cubeProbe = Com_GrowListElement(&tr.cubeProbes, j);

			// bind u_ColorMap
			GL_SelectTexture(0);
			GL_Bind(cubeProbe->cubemap);

			// set up the transformation matrix
			MatrixSetupTranslation(backEnd.orientation.transformMatrix, cubeProbe->origin[0], cubeProbe->origin[1], cubeProbe->origin[2]);
			MatrixMultiply(backEnd.viewParms.world.viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.modelViewMatrix);

			GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
			GLSL_SetUniform_ModelMatrix(&tr.reflectionShader_C, backEnd.orientation.transformMatrix);
			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.reflectionShader_C, glState.modelViewProjectionMatrix[glState.stackIndex]);

			tess.numIndexes = 0;
			tess.numVertexes = 0;

			VectorSet4(quadVerts[0], mins[0], mins[1], mins[2], 1);
			VectorSet4(quadVerts[1], mins[0], maxs[1], mins[2], 1);
			VectorSet4(quadVerts[2], mins[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[3], mins[0], mins[1], maxs[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			VectorSet4(quadVerts[0], maxs[0], mins[1], maxs[2], 1);
			VectorSet4(quadVerts[1], maxs[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[2], maxs[0], maxs[1], mins[2], 1);
			VectorSet4(quadVerts[3], maxs[0], mins[1], mins[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			VectorSet4(quadVerts[0], mins[0], mins[1], maxs[2], 1);
			VectorSet4(quadVerts[1], mins[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[2], maxs[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[3], maxs[0], mins[1], maxs[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			VectorSet4(quadVerts[0], maxs[0], mins[1], mins[2], 1);
			VectorSet4(quadVerts[1], maxs[0], maxs[1], mins[2], 1);
			VectorSet4(quadVerts[2], mins[0], maxs[1], mins[2], 1);
			VectorSet4(quadVerts[3], mins[0], mins[1], mins[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			VectorSet4(quadVerts[0], mins[0], mins[1], mins[2], 1);
			VectorSet4(quadVerts[1], mins[0], mins[1], maxs[2], 1);
			VectorSet4(quadVerts[2], maxs[0], mins[1], maxs[2], 1);
			VectorSet4(quadVerts[3], maxs[0], mins[1], mins[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			VectorSet4(quadVerts[0], maxs[0], maxs[1], mins[2], 1);
			VectorSet4(quadVerts[1], maxs[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[2], mins[0], maxs[1], maxs[2], 1);
			VectorSet4(quadVerts[3], mins[0], maxs[1], mins[2], 1);
			Tess_AddQuadStamp2WithNormals(quadVerts, colorWhite);

			Tess_UpdateVBOs(ATTR_POSITION | ATTR_NORMAL);
			Tess_DrawElements();

			tess.numIndexes = 0;
			tess.numVertexes = 0;
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}
#endif

	if(r_showBspNodes->integer)
	{
		bspNode_t      *node;
		link_t		   *l, *sentinel;

		if(tr.refdef.rdflags & (RDF_NOWORLDMODEL))
		{
			return;
		}

		GL_BindProgram(&tr.genericSingleShader);
		GL_State(GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE);
		GL_Cull(CT_TWO_SIDED);

		// set uniforms
		GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
		GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_CUSTOM_RGB);
		GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_CUSTOM);
		if(glConfig.vboVertexSkinningAvailable)
		{
			GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
		}
		GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
		GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.orientation.modelViewMatrix);
		GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

		// bind u_ColorMap
		GL_SelectTexture(0);
		GL_Bind(tr.whiteImage);
		GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

		GL_CheckErrors();

		if(r_dynamicBspOcclusionCulling->integer)
		{
			//sentinel = &tr.occlusionQueryList;
			sentinel = &tr.traversalStack;
		}
		else
		{
			sentinel = &tr.traversalStack;
		}

		for(l = sentinel->next; l != sentinel; l = l->next)
		{
			node = (bspNode_t *) l->data;

			if(!r_dynamicBspOcclusionCulling->integer)
			{
				if(node->contents != -1)
				{
					if(r_showBspNodes->integer == 3)
						continue;

					if(node->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorGreen);
					else
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorRed);
				}
				else
				{
					if(r_showBspNodes->integer == 2)
						continue;

					if(node->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorYellow);
					else
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorBlue);
				}
			}
			else
			{
				if(node->lastVisited[backEnd.viewParms.viewCount] != backEnd.viewParms.frameCount)
					continue;

				if(r_showBspNodes->integer == 5 && node->lastQueried[backEnd.viewParms.viewCount] != backEnd.viewParms.frameCount)
					continue;

				if(node->contents != -1)
				{
					if(r_showBspNodes->integer == 3)
						continue;

					//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
					if(node->visible[backEnd.viewParms.viewCount])
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorGreen);
					else
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorRed);
				}
				else
				{
					if(r_showBspNodes->integer == 2)
						continue;

					//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
					if(node->visible[backEnd.viewParms.viewCount])
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorYellow);
					else
						GLSL_SetUniform_Color(&tr.genericSingleShader, colorBlue);
				}

				if(r_showBspNodes->integer == 4)
				{
					GLSL_SetUniform_Color(&tr.genericSingleShader, g_color_table[ColorIndex(node->occlusionQueryNumbers[backEnd.viewParms.viewCount])]);
				}

				GL_CheckErrors();
			}

			if(node->contents != -1)
			{
				qglEnable(GL_POLYGON_OFFSET_FILL);
				qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
			}

			R_BindVBO(node->volumeVBO);
			R_BindIBO(node->volumeIBO);

			GL_VertexAttribsState(ATTR_POSITION);

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes = node->volumeIndexes;

			Tess_DrawElements();

			tess.numIndexes = 0;
			tess.numVertexes = 0;

			if(node->contents != -1)
			{
				qglDisable(GL_POLYGON_OFFSET_FILL);
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	GL_CheckErrors();
}

/*
==================
RB_RenderView
==================
*/
static void RB_RenderView(void)
{
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va
						 ("--- RB_RenderView( %i surfaces, %i interactions ) ---\n", backEnd.viewParms.numDrawSurfs,
						  backEnd.viewParms.numInteractions));
	}

	//ri.Error(ERR_FATAL, "test");

	GL_CheckErrors();

	backEnd.pc.c_surfaces += backEnd.viewParms.numDrawSurfs;

	if(DS_PREPASS_LIGHTING_ENABLED())
	{
		//
		// Deferred lighting path described by Wolfgang Engels
		//

		int             clearBits = 0;
		int             startTime = 0, endTime = 0;

		// sync with gl if needed
		if(r_finish->integer == 1 && !glState.finishCalled)
		{
			qglFinish();
			glState.finishCalled = qtrue;
		}
		if(r_finish->integer == 0)
		{
			glState.finishCalled = qtrue;
		}

		// we will need to change the projection matrix before drawing
		// 2D images again
		backEnd.projection2D = qfalse;

		// set the modelview matrix for the viewer
		SetViewportAndScissor();

		// ensures that depth writes are enabled for the depth clear
		GL_State(GLS_DEFAULT);

#if defined(OFFSCREEN_PREPASS_LIGHTING)
		// clear frame buffer objects
		R_BindFBO(tr.deferredRenderFBO);

		clearBits = GL_DEPTH_BUFFER_BIT;

		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits |= GL_COLOR_BUFFER_BIT;
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		qglClear(clearBits);

		R_BindFBO(tr.geometricRenderFBO);
		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits = GL_COLOR_BUFFER_BIT;
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		else
		{
			if(glConfig.framebufferBlitAvailable)
			{
				// copy color of the main context to deferredRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
		}
		qglClear(clearBits);

#else
		if(glConfig.framebufferObjectAvailable)
		{
			R_BindNullFBO();
		}

		// clear relevant buffers
		clearBits = GL_DEPTH_BUFFER_BIT;

		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		qglClear(clearBits);
#endif

		if((backEnd.refdef.rdflags & RDF_HYPERSPACE))
		{
			RB_Hyperspace();
			return;
		}
		else
		{
			backEnd.isHyperspace = qfalse;
		}


		glState.faceCulling = -1;	// force face culling to set next time

		// we will only draw a sun if there was sky rendered in this view
		backEnd.skyRenderedThisView = qfalse;

		GL_CheckErrors();

		RB_RenderDrawSurfacesIntoGeometricBuffer();


		// try to cull bsp nodes for the next frame using hardware occlusion queries
		/*
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_RenderBspOcclusionQueries();
		*/

		// try to cull lights using hardware occlusion queries
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_RenderLightOcclusionQueries();

		if(r_shadows->integer >= 4)
		{
			// render dynamic shadowing and lighting using shadow mapping
			RB_RenderInteractionsDeferredShadowMapped();
		}
		else
		{
			// render dynamic lighting
			RB_RenderInteractionsDeferred();
		}

#if !defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindNullFBO();

		// update light render image
		GL_SelectTexture(0);
		GL_Bind(tr.lightRenderFBOImage);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.lightRenderFBOImage->uploadWidth, tr.lightRenderFBOImage->uploadHeight);

		// restore color buffer
#if 1
		{
			matrix_t        ortho;

			// set 2D virtual screen size
			GL_PushMatrix();
			MatrixOrthogonalProjection(ortho, backEnd.viewParms.viewportX,
											backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
											backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
											-99999, 99999);
			GL_LoadProjectionMatrix(ortho);
			GL_LoadModelViewMatrix(matrixIdentity);

			GL_State(GLS_DEPTHTEST_DISABLE);
			GL_Cull(CT_TWO_SIDED);

			// enable shader, set arrays
			GL_BindProgram(&tr.screenShader);

			GLSL_SetUniform_ModelViewProjectionMatrix(&tr.screenShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

			GL_SelectTexture(0);
			GL_Bind(tr.currentRenderImage);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);

			// draw viewport
			Tess_InstantQuad(backEnd.viewParms.viewportVerts);

			// go back to 3D
			GL_PopMatrix();

			GL_CheckErrors();
		}
#endif
#endif

		if(r_speeds->integer == 9)
		{
			qglFinish();
			startTime = ri.Milliseconds();
		}

		// render opaque surfaces using the light buffer results
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_RenderDrawSurfaces(qtrue, qfalse);

		if(r_speeds->integer == 9)
		{
			qglFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_forwardLightingTime = endTime - startTime;
		}

		// draw everything that is translucent
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_RenderDrawSurfaces(qfalse, qfalse);

		if(r_speeds->integer == 9)
		{
			qglFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_forwardTranslucentTime = endTime - startTime;
		}

		// wait until all bsp node occlusion queries are back
		/*
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_CollectBspOcclusionQueries();
		*/

		// render debug information
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
		RB_RenderDebugUtils();

		// scale down rendered HDR scene to 1 / 4th
		if(HDR_ENABLED())
		{
			if(glConfig.framebufferBlitAvailable)
			{
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_LINEAR);

				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, 64, 64,
									   GL_COLOR_BUFFER_BIT,
									   GL_LINEAR);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}

			RB_CalculateAdaptation();
		}
		else
		{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
			if(glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to downScaleFBO_quarter
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_LINEAR);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}
#else
#if 0
			// FIXME: this trashes the OpenGL context for an unknown reason
			if(glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable)
			{
				// copy main context to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
			else
#endif
			{
				// FIXME add non EXT_framebuffer_blit code
			}
#endif
		}

		GL_CheckErrors();

#if defined(OFFSCREEN_PREPASS_LIGHTING)
		// render bloom post process effect
		RB_RenderBloom();

		// copy offscreen rendered scene to the current OpenGL context
		RB_RenderDeferredShadingResultToFrameBuffer();
#endif

		if(backEnd.viewParms.isPortal)
		{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
			if(glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
									   0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
			else
			{
				// capture current color buffer
				GL_SelectTexture(0);
				GL_Bind(tr.portalRenderImage);
				qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			}
#else
#if 0
			// FIXME: this trashes the OpenGL context for an unknown reason
			if(glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable)
			{
				// copy main context to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
			else
#endif
			{
				// capture current color buffer
				GL_SelectTexture(0);
				GL_Bind(tr.portalRenderImage);
				qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			}
#endif
			backEnd.pc.c_portals++;
		}
	}
	else if(DS_STANDARD_ENABLED())
	{
		//
		// Deferred shading path
		//

		int             clearBits = 0;

		// sync with gl if needed
		if(r_finish->integer == 1 && !glState.finishCalled)
		{
			qglFinish();
			glState.finishCalled = qtrue;
		}
		if(r_finish->integer == 0)
		{
			glState.finishCalled = qtrue;
		}

		// we will need to change the projection matrix before drawing
		// 2D images again
		backEnd.projection2D = qfalse;

		// set the modelview matrix for the viewer
		SetViewportAndScissor();

		// ensures that depth writes are enabled for the depth clear
		GL_State(GLS_DEFAULT);

		// clear frame buffer objects
		R_BindFBO(tr.deferredRenderFBO);
		//qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		clearBits = GL_DEPTH_BUFFER_BIT;

		/*
		   if(r_measureOverdraw->integer || r_shadows->integer == 3)
		   {
		   clearBits |= GL_STENCIL_BUFFER_BIT;
		   }
		 */
		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits |= GL_COLOR_BUFFER_BIT;
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		qglClear(clearBits);

		R_BindFBO(tr.geometricRenderFBO);
		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits = GL_COLOR_BUFFER_BIT;
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		else
		{
			if(glConfig.framebufferBlitAvailable)
			{
				// copy color of the main context to deferredRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
		}
		qglClear(clearBits);

		if((backEnd.refdef.rdflags & RDF_HYPERSPACE))
		{
			RB_Hyperspace();
			return;
		}
		else
		{
			backEnd.isHyperspace = qfalse;
		}

		glState.faceCulling = -1;	// force face culling to set next time

		// we will only draw a sun if there was sky rendered in this view
		backEnd.skyRenderedThisView = qfalse;

		GL_CheckErrors();

#if defined(DEFERRED_SHADING_Z_PREPASS)
		// draw everything that is opaque
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderDrawSurfaces(qtrue, qfalse);
#endif

		RB_RenderDrawSurfacesIntoGeometricBuffer();

		// try to cull bsp nodes for the next frame using hardware occlusion queries
		/*
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderBspOcclusionQueries();
		*/

		// try to cull lights using hardware occlusion queries
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderLightOcclusionQueries();

		if(!r_showDeferredRender->integer)
		{
			if(r_shadows->integer >= 4)
			{
				// render dynamic shadowing and lighting using shadow mapping
				RB_RenderInteractionsDeferredShadowMapped();
			}
			else
			{
				// render dynamic lighting
				RB_RenderInteractionsDeferred();
			}
		}

		// draw everything that is translucent
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderDrawSurfaces(qfalse, qfalse);

		// render global fog
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderUniformFog();

		// wait until all bsp node occlusion queries are back
		/*
		R_BindFBO(tr.deferredRenderFBO);
		RB_CollectBspOcclusionQueries();
		*/

		// render debug information
		R_BindFBO(tr.deferredRenderFBO);
		RB_RenderDebugUtils();

		// scale down rendered HDR scene to 1 / 4th
		if(r_hdrRendering->integer)
		{
			if(glConfig.framebufferBlitAvailable)
			{
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_LINEAR);

				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, 64, 64,
									   GL_COLOR_BUFFER_BIT,
									   GL_LINEAR);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}

			RB_CalculateAdaptation();
		}
		else
		{
			if(glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to downScaleFBO_quarter
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_LINEAR);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}
		}

		GL_CheckErrors();

		// render bloom post process effect
		RB_RenderBloom();

		// copy offscreen rendered scene to the current OpenGL context
		RB_RenderDeferredShadingResultToFrameBuffer();

		if(backEnd.viewParms.isPortal)
		{
			if(glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
									   0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
			else
			{
				// capture current color buffer
				GL_SelectTexture(0);
				GL_Bind(tr.portalRenderImage);
				qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			}
			backEnd.pc.c_portals++;
		}
	}
	else
	{
		//
		// Forward shading path
		//

		int             clearBits = 0;
		int             startTime = 0, endTime = 0;

		// sync with gl if needed
		if(r_finish->integer == 1 && !glState.finishCalled)
		{
			qglFinish();
			glState.finishCalled = qtrue;
		}
		if(r_finish->integer == 0)
		{
			glState.finishCalled = qtrue;
		}

		// disable offscreen rendering
		if(glConfig.framebufferObjectAvailable)
		{
			if(r_hdrRendering->integer && glConfig.textureFloatAvailable)
				R_BindFBO(tr.deferredRenderFBO);
			else
				R_BindNullFBO();
		}

		// we will need to change the projection matrix before drawing
		// 2D images again
		backEnd.projection2D = qfalse;

		// set the modelview matrix for the viewer
		SetViewportAndScissor();

		// ensures that depth writes are enabled for the depth clear
		GL_State(GLS_DEFAULT);

		// clear relevant buffers
		clearBits = GL_DEPTH_BUFFER_BIT;

		if(r_measureOverdraw->integer || r_shadows->integer == 3)
		{
			clearBits |= GL_STENCIL_BUFFER_BIT;
		}

		if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
		{
			clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
			GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// FIXME: get color of sky
		}
		else
		{
			if(HDR_ENABLED())
			{
				// copy color of the main context to deferredRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
		}
		qglClear(clearBits);

		if((backEnd.refdef.rdflags & RDF_HYPERSPACE))
		{
			RB_Hyperspace();
			return;
		}
		else
		{
			backEnd.isHyperspace = qfalse;
		}

		glState.faceCulling = -1;	// force face culling to set next time

		// we will only draw a sun if there was sky rendered in this view
		backEnd.skyRenderedThisView = qfalse;

		GL_CheckErrors();

		if(r_speeds->integer == 9)
		{
			qglFinish();
			startTime = ri.Milliseconds();
		}

		// draw everything that is opaque into black so we can benefit from early-z rejections later
		//RB_RenderDrawSurfaces(qtrue, qtrue);

		// draw everything that is opaque
		RB_RenderDrawSurfaces(qtrue, qfalse);

		// try to cull bsp nodes for the next frame using hardware occlusion queries
		//RB_RenderBspOcclusionQueries();

		// render ambient occlusion process effect
		// Tr3B: needs way more work RB_RenderScreenSpaceAmbientOcclusion(qfalse);

		if(r_speeds->integer == 9)
		{
			qglFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_forwardAmbientTime = endTime - startTime;
		}

		// try to cull lights using hardware occlusion queries
		RB_RenderLightOcclusionQueries();

		if(r_shadows->integer >= 4)
		{
			// render dynamic shadowing and lighting using shadow mapping
			RB_RenderInteractionsShadowMapped();

			// render player shadows if any
			//RB_RenderInteractionsDeferredInverseShadows();
		}
		else if(r_shadows->integer == 3)
		{
			// render dynamic shadowing and lighting using stencil shadow volumes
			RB_RenderInteractionsStencilShadowed();
		}
		else
		{
			// render dynamic lighting
			RB_RenderInteractions();
		}

		if(r_hdrRendering->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
			R_BindFBO(tr.deferredRenderFBO);

		// draw everything that is translucent
		RB_RenderDrawSurfaces(qfalse, qfalse);

		// render global fog post process effect
		RB_RenderUniformFog(qfalse);

		// scale down rendered HDR scene to 1 / 4th
		if(r_hdrRendering->integer && glConfig.textureFloatAvailable && glConfig.framebufferObjectAvailable)
		{
			if(glConfig.framebufferBlitAvailable)
			{
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_LINEAR);

				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, 64, 64,
									   GL_COLOR_BUFFER_BIT,
									   GL_LINEAR);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}

			RB_CalculateAdaptation();
		}
		else
		{
			/*
			Tr3B: FIXME this causes: caught OpenGL error:
			GL_INVALID_OPERATION in file code/renderer/tr_backend.c line 6479

			if(glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to downScaleFBO_quarter
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
										0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
										GL_COLOR_BUFFER_BIT,
										GL_NEAREST);
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}
			*/
		}

		GL_CheckErrors();
#ifdef EXPERIMENTAL
		// render depth of field post process effect
		RB_RenderDepthOfField(qfalse);
#endif
		// render bloom post process effect
		RB_RenderBloom();

		// copy offscreen rendered HDR scene to the current OpenGL context
		RB_RenderDeferredHDRResultToFrameBuffer();

		// render rotoscope post process effect
		RB_RenderRotoscope();

#if 0
		// add the sun flare
		RB_DrawSun();
#endif

#if 0
		// add light flares on lights that aren't obscured
		RB_RenderFlares();
#endif

		// wait until all bsp node occlusion queries are back
		//RB_CollectBspOcclusionQueries();

		// render debug information
		RB_RenderDebugUtils();

		if(backEnd.viewParms.isPortal)
		{
			if(r_hdrRendering->integer && glConfig.textureFloatAvailable && glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable)
			{
				// copy deferredRenderFBO to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
				                       0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
				                       GL_COLOR_BUFFER_BIT,
				                       GL_NEAREST);
			}
#if 0
			// FIXME: this trashes the OpenGL context for an unknown reason
			else if(glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable)
			{
				// copy main context to portalRenderFBO
				qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
				qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
				qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   0, 0, glConfig.vidWidth, glConfig.vidHeight,
									   GL_COLOR_BUFFER_BIT,
									   GL_NEAREST);
			}
#endif
			else
			{
				// capture current color buffer
				GL_SelectTexture(0);
				GL_Bind(tr.portalRenderImage);
				qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			}
			backEnd.pc.c_portals++;
		}

#if 0
		if(r_dynamicBspOcclusionCulling->integer)
		{
			// copy depth of the main context to deferredRenderFBO
			qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
			qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer);
			qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
								   0, 0, glConfig.vidWidth, glConfig.vidHeight,
								   GL_DEPTH_BUFFER_BIT,
								   GL_NEAREST);
		}
#endif
	}

	// render chromatric aberration
	RB_CameraPostFX();

	// copy to given byte buffer that is NOT a FBO
	if(tr.refdef.pixelTarget != NULL)
	{
		int             i;

		// need to convert Y axis
#if 0
		qglReadPixels(0, 0, tr.refdef.pixelTargetWidth, tr.refdef.pixelTargetHeight, GL_RGBA, GL_UNSIGNED_BYTE, tr.refdef.pixelTarget);
#else
		// Bugfix: drivers absolutely hate running in high res and using qglReadPixels near the top or bottom edge.
		// Soo.. lets do it in the middle.
		qglReadPixels(glConfig.vidWidth / 2, glConfig.vidHeight / 2, tr.refdef.pixelTargetWidth, tr.refdef.pixelTargetHeight, GL_RGBA,
					  GL_UNSIGNED_BYTE, tr.refdef.pixelTarget);
#endif

		for(i = 0; i < tr.refdef.pixelTargetWidth * tr.refdef.pixelTargetHeight; i++)
		{
			tr.refdef.pixelTarget[(i * 4) + 3] = 255;	//set the alpha pure white
		}
	}

	GL_CheckErrors();

	backEnd.pc.c_views++;
}


/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte * data, int client, qboolean dirty)
{
	int             i, j;
	int             start, end;

	if(!tr.registered)
	{
		return;
	}
	R_SyncRenderThread();

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = end = 0;
	if(r_speeds->integer)
	{
		qglFinish();
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for(i = 0; (1 << i) < cols; i++)
	{
	}
	for(j = 0; (1 << j) < rows; j++)
	{
	}
	if((1 << i) != cols || (1 << j) != rows)
	{
		ri.Error(ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}


	RB_SetGL2D();

	qglVertexAttrib4fARB(ATTR_INDEX_NORMAL, 0, 0, 1, 1);
	qglVertexAttrib4fARB(ATTR_INDEX_COLOR, tr.identityLight, tr.identityLight, tr.identityLight, 1);

	GL_BindProgram(&tr.genericSingleShader);

	// set uniforms
	GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
	GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
	GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
	//GLSL_SetUniform_Color(&tr.genericSingleShader, colorWhite);
	if(glConfig.vboVertexSkinningAvailable)
	{
		GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
	}
	GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
	GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);
	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

	// bind u_ColorMap
	GL_SelectTexture(0);
	GL_Bind(tr.scratchImage[client]);
	GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if(cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height)
	{
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;

		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	else
	{
		if(dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}

	if(r_speeds->integer)
	{
		qglFinish();
		end = ri.Milliseconds();
		ri.Printf(PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	/*
	   qglBegin(GL_QUADS);
	   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0.5f / cols, 0.5f / rows, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, (cols - 0.5f) / cols, 0.5f / rows, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, (cols - 0.5f) / cols, (rows - 0.5f) / rows, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y + h, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0.5f / cols, (rows - 0.5f) / rows, 0, 1);
	   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y + h, 0, 1);
	   qglEnd();
	 */

	tess.numVertexes = 0;
	tess.numIndexes = 0;

	tess.xyz[tess.numVertexes][0] = x;
	tess.xyz[tess.numVertexes][1] = y;
	tess.xyz[tess.numVertexes][2] = 0;
	tess.xyz[tess.numVertexes][3] = 1;
	tess.texCoords[tess.numVertexes][0] = 0.5f / cols;
	tess.texCoords[tess.numVertexes][1] = 0.5f / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = x + w;
	tess.xyz[tess.numVertexes][1] = y;
	tess.xyz[tess.numVertexes][2] = 0;
	tess.xyz[tess.numVertexes][3] = 1;
	tess.texCoords[tess.numVertexes][0] = (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][1] = 0.5f / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = x + w;
	tess.xyz[tess.numVertexes][1] = y + h;
	tess.xyz[tess.numVertexes][2] = 0;
	tess.xyz[tess.numVertexes][3] = 1;
	tess.texCoords[tess.numVertexes][0] = (cols - 0.5f) / cols;
	tess.texCoords[tess.numVertexes][1] = (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = x;
	tess.xyz[tess.numVertexes][1] = y + h;
	tess.xyz[tess.numVertexes][2] = 0;
	tess.xyz[tess.numVertexes][3] = 1;
	tess.texCoords[tess.numVertexes][0] = 0.5f / cols;
	tess.texCoords[tess.numVertexes][1] = (rows - 0.5f) / rows;
	tess.texCoords[tess.numVertexes][2] = 0;
	tess.texCoords[tess.numVertexes][3] = 1;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	Tess_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);

	Tess_DrawElements();

	tess.numVertexes = 0;
	tess.numIndexes = 0;

	GL_CheckErrors();
}

void RE_UploadCinematic(int w, int h, int cols, int rows, const byte * data, int client, qboolean dirty)
{
	GL_Bind(tr.scratchImage[client]);

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if(cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height)
	{
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;

		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		qglTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colorBlack);
	}
	else
	{
		if(dirty)
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}

	GL_CheckErrors();
}


/*
=============
RB_SetColor
=============
*/
const void     *RB_SetColor(const void *data)
{
	const setColorCommand_t *cmd;

	GLimp_LogComment("--- RB_SetColor ---\n");

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0];
	backEnd.color2D[1] = cmd->color[1];
	backEnd.color2D[2] = cmd->color[2];
	backEnd.color2D[3] = cmd->color[3];

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void     *RB_StretchPic(const void *data)
{
	int             i;
	const stretchPicCommand_t *cmd;
	shader_t       *shader;
	int             numVerts, numIndexes;

	GLimp_LogComment("--- RB_StretchPic ---\n");

	cmd = (const stretchPicCommand_t *)data;

	if(!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if(shader != tess.surfaceShader)
	{
		if(tess.numIndexes)
		{
			Tess_End();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin(Tess_StageIteratorGeneric, shader, NULL, qfalse, qfalse, -1);
	}

	Tess_CheckOverflow(4, 6);
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[numIndexes] = numVerts + 3;
	tess.indexes[numIndexes + 1] = numVerts + 0;
	tess.indexes[numIndexes + 2] = numVerts + 2;
	tess.indexes[numIndexes + 3] = numVerts + 2;
	tess.indexes[numIndexes + 4] = numVerts + 0;
	tess.indexes[numIndexes + 5] = numVerts + 1;


	for(i = 0; i < 4; i++)
	{
		tess.colors[numVerts + i][0] = backEnd.color2D[0];
		tess.colors[numVerts + i][1] = backEnd.color2D[1];
		tess.colors[numVerts + i][2] = backEnd.color2D[2];
		tess.colors[numVerts + i][3] = backEnd.color2D[3];
	}

	tess.xyz[numVerts][0] = cmd->x;
	tess.xyz[numVerts][1] = cmd->y;
	tess.xyz[numVerts][2] = 0;
	tess.xyz[numVerts][3] = 1;

	tess.texCoords[numVerts][0] = cmd->s1;
	tess.texCoords[numVerts][1] = cmd->t1;
	tess.texCoords[numVerts][2] = 0;
	tess.texCoords[numVerts][3] = 1;

	tess.xyz[numVerts + 1][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 1][1] = cmd->y;
	tess.xyz[numVerts + 1][2] = 0;
	tess.xyz[numVerts + 1][3] = 1;

	tess.texCoords[numVerts + 1][0] = cmd->s2;
	tess.texCoords[numVerts + 1][1] = cmd->t1;
	tess.texCoords[numVerts + 1][2] = 0;
	tess.texCoords[numVerts + 1][3] = 1;

	tess.xyz[numVerts + 2][0] = cmd->x + cmd->w;
	tess.xyz[numVerts + 2][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 2][2] = 0;
	tess.xyz[numVerts + 2][3] = 1;

	tess.texCoords[numVerts + 2][0] = cmd->s2;
	tess.texCoords[numVerts + 2][1] = cmd->t2;
	tess.texCoords[numVerts + 2][2] = 0;
	tess.texCoords[numVerts + 2][3] = 1;

	tess.xyz[numVerts + 3][0] = cmd->x;
	tess.xyz[numVerts + 3][1] = cmd->y + cmd->h;
	tess.xyz[numVerts + 3][2] = 0;
	tess.xyz[numVerts + 3][3] = 1;

	tess.texCoords[numVerts + 3][0] = cmd->s1;
	tess.texCoords[numVerts + 3][1] = cmd->t2;
	tess.texCoords[numVerts + 3][2] = 0;
	tess.texCoords[numVerts + 3][3] = 1;

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawView
=============
*/
const void     *RB_DrawView(const void *data)
{
	const drawViewCommand_t *cmd;

	GLimp_LogComment("--- RB_DrawView ---\n");

	// finish any 2D drawing if needed
	if(tess.numIndexes)
	{
		Tess_End();
	}

	cmd = (const drawViewCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderView();

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer
=============
*/
const void     *RB_DrawBuffer(const void *data)
{
	const drawBufferCommand_t *cmd;

	GLimp_LogComment("--- RB_DrawBuffer ---\n");

	cmd = (const drawBufferCommand_t *)data;

	GL_DrawBuffer(cmd->buffer);

	// clear screen for debugging
	if(r_clear->integer)
	{
//      GL_ClearColor(1, 0, 0.5, 1);
		GL_ClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages(void)
{
	int             i;
	image_t        *image;
	float           x, y, w, h;
	vec4_t          quadVerts[4];
	int             start, end;

	GLimp_LogComment("--- RB_ShowImages ---\n");

	if(!backEnd.projection2D)
	{
		RB_SetGL2D();
	}

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	GL_BindProgram(&tr.genericSingleShader);
	GL_Cull(CT_TWO_SIDED);

	// set uniforms
	GLSL_SetUniform_TCGen_Environment(&tr.genericSingleShader,  qfalse);
	GLSL_SetUniform_ColorGen(&tr.genericSingleShader, CGEN_VERTEX);
	GLSL_SetUniform_AlphaGen(&tr.genericSingleShader, AGEN_VERTEX);
	if(glConfig.vboVertexSkinningAvailable)
	{
		GLSL_SetUniform_VertexSkinning(&tr.genericSingleShader, qfalse);
	}
	GLSL_SetUniform_DeformGen(&tr.genericSingleShader, DGEN_NONE);
	GLSL_SetUniform_AlphaTest(&tr.genericSingleShader, 0);
	GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

	GL_SelectTexture(0);

	start = ri.Milliseconds();

	for(i = 0; i < tr.images.currentElements; i++)
	{
		image = Com_GrowListElement(&tr.images, i);

		/*
		   if(image->bits & (IF_RGBA16F | IF_RGBA32F | IF_LA16F | IF_LA32F))
		   {
		   // don't render float textures using FFP
		   continue;
		   }
		 */

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if(r_showImages->integer == 2)
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		// bind u_ColorMap
		GL_Bind(image);

		VectorSet4(quadVerts[0], x, y, 0, 1);
		VectorSet4(quadVerts[1], x + w, y, 0, 1);
		VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
		VectorSet4(quadVerts[3], x, y + h, 0, 1);

		Tess_InstantQuad(quadVerts);

		/*
		   qglBegin(GL_QUADS);
		   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0, 0, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 1, 0, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 1, 1, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y + h, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0, 1, 0, 1);
		   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y + h, 0, 1);
		   qglEnd();
		 */
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "%i msec to draw all images\n", end - start);

	GL_CheckErrors();
}

/*
=============
RB_SwapBuffers
=============
*/
const void     *RB_SwapBuffers(const void *data)
{
	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
	{
		Tess_End();
	}

	// texture swapping test
	if(r_showImages->integer)
	{
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if(r_measureOverdraw->integer)
	{
		int             i;
		long            sum = 0;
		unsigned char  *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight);
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

		for(i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
		{
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory(stencilReadback);
	}


	if(!glState.finishCalled)
	{
		qglFinish();
	}

	GLimp_LogComment("***************** RB_SwapBuffers *****************\n\n\n");

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands(const void *data)
{
	int             t1, t2;

	GLimp_LogComment("--- RB_ExecuteRenderCommands ---\n");

	t1 = ri.Milliseconds();

	if(!r_smp->integer || data == backEndData[0]->commands.cmds)
	{
		backEnd.smpFrame = 0;
	}
	else
	{
		backEnd.smpFrame = 1;
	}

	while(1)
	{
		switch (*(const int *)data)
		{
			case RC_SET_COLOR:
				data = RB_SetColor(data);
				break;
			case RC_STRETCH_PIC:
				data = RB_StretchPic(data);
				break;
			case RC_DRAW_VIEW:
				data = RB_DrawView(data);
				break;
			case RC_DRAW_BUFFER:
				data = RB_DrawBuffer(data);
				break;
			case RC_SWAP_BUFFERS:
				data = RB_SwapBuffers(data);
				break;
			case RC_SCREENSHOT:
				data = RB_TakeScreenshotCmd(data);
				break;
			case RC_VIDEOFRAME:
				data = RB_TakeVideoFrameCmd(data);
				break;

			case RC_END_OF_LIST:
			default:
				// stop rendering on this thread
				t2 = ri.Milliseconds();
				backEnd.pc.msec = t2 - t1;
				return;
		}
	}
}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread(void)
{
	const void     *data;

	// wait for either a rendering command or a quit command
	while(1)
	{
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if(!data)
		{
			return;				// all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands(data);

		renderThreadActive = qfalse;
	}
}
