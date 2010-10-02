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
// tr_world.c
#include <hat/renderer/tr_local.h>

/*
=================
R_CullTriSurf

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean R_CullTriSurf(srfTriangles_t * cv)
{
	int             boxCull;

	boxCull = R_CullLocalBox(cv->bounds);

	if(boxCull == CULL_OUT)
	{
		return qtrue;
	}
	return qfalse;
}

/*
=================
R_CullGrid

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean R_CullGrid(srfGridMesh_t * cv)
{
	int             boxCull;
	int             sphereCull;

	if(r_nocurves->integer)
	{
		return qtrue;
	}

	if(tr.currentEntity != &tr.worldEntity)
	{
		sphereCull = R_CullLocalPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	else
	{
		sphereCull = R_CullPointAndRadius(cv->localOrigin, cv->meshRadius);
	}
	boxCull = CULL_OUT;

	// check for trivial reject
	if(sphereCull == CULL_OUT)
	{
		tr.pc.c_sphere_cull_patch_out++;
		return qtrue;
	}
	// check bounding box if necessary
	else if(sphereCull == CULL_CLIP)
	{
		tr.pc.c_sphere_cull_patch_clip++;

		boxCull = R_CullLocalBox(cv->meshBounds);

		if(boxCull == CULL_OUT)
		{
			tr.pc.c_box_cull_patch_out++;
			return qtrue;
		}
		else if(boxCull == CULL_IN)
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return qfalse;
}


/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean R_CullSurface(surfaceType_t * surface, shader_t * shader)
{
#if 1
	srfSurfaceFace_t *sface;
	float           d;

	if(r_nocull->integer)
	{
		return qfalse;
	}

	if(*surface == SF_GRID)
	{
		return R_CullGrid((srfGridMesh_t *) surface);
	}

	if(*surface == SF_TRIANGLES)
	{
		return R_CullTriSurf((srfTriangles_t *) surface);
	}

	if(*surface != SF_FACE)
	{
		return qfalse;
	}

	// now it must be a SF_FACE
	sface = (srfSurfaceFace_t *) surface;

	if(shader->isPortal)
	{
		if(R_CullLocalBox(sface->bounds) == CULL_OUT)
		{
			return qtrue;
		}
	}

	if(shader->cullType == CT_TWO_SIDED)
	{
		return qfalse;
	}

	// face culling
	if(!r_facePlaneCull->integer)
	{
		return qfalse;
	}


	d = DotProduct(tr.orientation.viewOrigin, sface->plane.normal);

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here
	if(shader->cullType == CT_FRONT_SIDED)
	{
		if(d < sface->plane.dist - 8)
		{
			return qtrue;
		}
	}
	else
	{
		if(d > sface->plane.dist + 8)
		{
			return qtrue;
		}
	}

	return qfalse;
#else
	return qfalse;
#endif
}

// *INDENT-OFF*
static qboolean R_LightFace(srfSurfaceFace_t * face, trRefLight_t  * light, byte * cubeSideBits)
{
	// do a quick AABB cull
	if(!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], face->bounds[0], face->bounds[1]))
	{
		return qfalse;
	}

	// do a more expensive and precise light frustum cull
	if(!r_noLightFrustums->integer)
	{
		if(R_CullLightWorldBounds(light, face->bounds) == CULL_OUT)
		{
			return qfalse;
		}
	}

	if(r_cullShadowPyramidFaces->integer)
	{
		*cubeSideBits = R_CalcLightCubeSideBits(light, face->bounds);
	}
	return qtrue;
}
// *INDENT-ON*

static int R_LightGrid(srfGridMesh_t * grid, trRefLight_t * light, byte * cubeSideBits)
{
	// do a quick AABB cull
	if(!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], grid->meshBounds[0], grid->meshBounds[1]))
	{
		return qfalse;
	}

	// do a more expensive and precise light frustum cull
	if(!r_noLightFrustums->integer)
	{
		if(R_CullLightWorldBounds(light, grid->meshBounds) == CULL_OUT)
		{
			return qfalse;
		}
	}

	if(r_cullShadowPyramidCurves->integer)
	{
		*cubeSideBits = R_CalcLightCubeSideBits(light, grid->meshBounds);
	}
	return qtrue;
}


static int R_LightTrisurf(srfTriangles_t * tri, trRefLight_t * light, byte * cubeSideBits)
{
	// do a quick AABB cull
	if(!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], tri->bounds[0], tri->bounds[1]))
	{
		return qfalse;
	}

	// do a more expensive and precise light frustum cull
	if(!r_noLightFrustums->integer)
	{
		if(R_CullLightWorldBounds(light, tri->bounds) == CULL_OUT)
		{
			return qfalse;
		}
	}

	if(r_cullShadowPyramidTriangles->integer)
	{
		*cubeSideBits = R_CalcLightCubeSideBits(light, tri->bounds);
	}
	return qtrue;
}


/*
======================
R_AddInteractionSurface
======================
*/
static void R_AddInteractionSurface(bspSurface_t * surf, trRefLight_t * light)
{
	qboolean        intersects;
	interactionType_t iaType = IA_DEFAULT;
	byte            cubeSideBits = CUBESIDE_CLIPALL;

	// Tr3B - this surface is maybe not in this view but it may still cast a shadow
	// into this view
	if(surf->viewCount != tr.viewCountNoReset)
	{
		if(r_shadows->integer <= 2 || light->l.noShadows)
			return;
		else
			iaType = IA_SHADOWONLY;
	}

	if(surf->lightCount == tr.lightCount)
	{
		return;					// already checked this surface
	}
	surf->lightCount = tr.lightCount;

	if(r_vboDynamicLighting->integer && !surf->shader->isSky && !surf->shader->isPortal && !ShaderRequiresCPUDeforms(surf->shader))
		return;

	//  skip all surfaces that don't matter for lighting only pass
	if(surf->shader->isSky || (!surf->shader->interactLight && surf->shader->noShadows))
		return;

	if(*surf->data == SF_FACE)
	{
		intersects = R_LightFace((srfSurfaceFace_t *) surf->data, light, &cubeSideBits);
	}
	else if(*surf->data == SF_GRID)
	{
		intersects = R_LightGrid((srfGridMesh_t *) surf->data, light, &cubeSideBits);
	}
	else if(*surf->data == SF_TRIANGLES)
	{
		intersects = R_LightTrisurf((srfTriangles_t *) surf->data, light, &cubeSideBits);
	}
	else
	{
		intersects = qfalse;
	}

	if(intersects)
	{
		R_AddLightInteraction(light, surf->data, surf->shader, cubeSideBits, iaType);

		if(light->isStatic)
			tr.pc.c_slightSurfaces++;
		else
			tr.pc.c_dlightSurfaces++;
	}
	else
	{
		if(!light->isStatic)
			tr.pc.c_dlightSurfacesCulled++;
	}
}

/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(bspSurface_t * surf)
{
	shader_t       *shader;

	if(surf->viewCount == tr.viewCountNoReset)
		return;
	surf->viewCount = tr.viewCountNoReset;

	shader = surf->shader;

	if(r_mergeClusterSurfaces->integer &&
		!r_dynamicBspOcclusionCulling->integer &&
		((r_mergeClusterFaces->integer && *surf->data == SF_FACE) ||
		(r_mergeClusterCurves->integer && *surf->data == SF_GRID) ||
		(r_mergeClusterTriangles->integer && *surf->data == SF_TRIANGLES)) &&
		!shader->isSky && !shader->isPortal && !ShaderRequiresCPUDeforms(shader))
		return;

	// try to cull before lighting or adding
	if(R_CullSurface(surf->data, surf->shader))
	{
		return;
	}

	R_AddDrawSurf(surf->data, surf->shader, surf->lightmapNum);
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
======================
R_AddBrushModelSurface
======================
*/
static void R_AddBrushModelSurface(bspSurface_t * surf)
{
	if(surf->viewCount == tr.viewCountNoReset)
	{
		return;					// already in this view
	}
	surf->viewCount = tr.viewCountNoReset;

	// try to cull before lighting or adding
	if(R_CullSurface(surf->data, surf->shader))
	{
		return;
	}

	R_AddDrawSurf(surf->data, surf->shader, -1);//surf->lightmapNum);
}

/*
=================
R_AddBSPModelSurfaces
=================
*/
void R_AddBSPModelSurfaces(trRefEntity_t * ent)
{
	bspModel_t     *bspModel;
	model_t        *pModel;
	int             i;
	vec3_t          v;
	vec3_t          transformed;

	pModel = R_GetModelByHandle(ent->e.hModel);
	bspModel = pModel->bsp;

	// copy local bounds
	for(i = 0; i < 3; i++)
	{
		ent->localBounds[0][i] = bspModel->bounds[0][i];
		ent->localBounds[1][i] = bspModel->bounds[1][i];
	}

	// setup world bounds for intersection tests
	ClearBounds(ent->worldBounds[0], ent->worldBounds[1]);

	for(i = 0; i < 8; i++)
	{
		v[0] = ent->localBounds[i & 1][0];
		v[1] = ent->localBounds[(i >> 1) & 1][1];
		v[2] = ent->localBounds[(i >> 2) & 1][2];

		// transform local bounds vertices into world space
		R_LocalPointToWorld(v, transformed);

		AddPointToBounds(transformed, ent->worldBounds[0], ent->worldBounds[1]);
	}

	ent->cull = R_CullLocalBox(bspModel->bounds);
	if(ent->cull == CULL_OUT)
	{
		return;
	}

	// Tr3B: BSP inline models should always use vertex lighting
	R_SetupEntityLighting(&tr.refdef, ent);

	if(r_vboModels->integer && bspModel->numVBOSurfaces)
	{
		int             i;
		srfVBOMesh_t   *vboSurface;

		for(i = 0; i < bspModel->numVBOSurfaces; i++)
		{
			vboSurface = bspModel->vboSurfaces[i];

			R_AddDrawSurf((void *)vboSurface, vboSurface->shader, -1);//vboSurface->lightmapNum);
		}
	}
	else
	{
		for(i = 0; i < bspModel->numSurfaces; i++)
		{
			R_AddBrushModelSurface(bspModel->firstSurface + i);
		}
	}
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/




/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode(bspNode_t * node, int planeBits)
{
	do
	{
		// if the node wasn't marked as potentially visible, exit
		if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
		{
			return;
		}

		if(node->contents != -1 && !node->numMarkSurfaces)
		{
			// don't waste time dealing with this empty leaf
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?
		if(!r_nocull->integer)
		{
			int             i;
			int             r;

			for(i = 0; i < FRUSTUM_PLANES; i++)
			{
				if(planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
					if(r == 2)
					{
						return;	// culled
					}
					if(r == 1)
					{
						planeBits &= ~(1 << i);	// all descendants will also be in front
					}
				}
			}
		}

		InsertLink(&node->visChain, &tr.traversalStack);

		if(node->contents != -1)
		{
			break;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode(node->children[0], planeBits);

		// tail recurse
		node = node->children[1];
	} while(1);

	{
		// leaf node, so add mark surfaces
		int             c;
		bspSurface_t   *surf, **mark;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		if(node->mins[0] < tr.viewParms.visBounds[0][0])
		{
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if(node->mins[1] < tr.viewParms.visBounds[0][1])
		{
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if(node->mins[2] < tr.viewParms.visBounds[0][2])
		{
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if(node->maxs[0] > tr.viewParms.visBounds[1][0])
		{
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if(node->maxs[1] > tr.viewParms.visBounds[1][1])
		{
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if(node->maxs[2] > tr.viewParms.visBounds[1][2])
		{
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}


		// add the individual surfaces
		mark = node->markSurfaces;
		c = node->numMarkSurfaces;
		while(c--)
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_AddWorldSurface(surf);
			mark++;
		}
	}
}

/*
================
R_RecursiveInteractionNode
================
*/
static void R_RecursiveInteractionNode(bspNode_t * node, trRefLight_t * light, int planeBits)
{
	int             i;
	int             r;

	// if the node wasn't marked as potentially visible, exit
	if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
	{
		return;
	}

	// light already hit node
	if(node->lightCount == tr.lightCount)
	{
		return;
	}
	node->lightCount = tr.lightCount;

	// if the bounding volume is outside the frustum, nothing
	// inside can be visible OPTIMIZE: don't do this all the way to leafs?

	// Tr3B - even surfaces that belong to nodes that are outside of the view frustum
	// can cast shadows into the view frustum
	if(!r_nocull->integer && r_shadows->integer <= 2)
	{
		for(i = 0; i < FRUSTUM_PLANES; i++)
		{
			if(planeBits & (1 << i))
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);

				if(r == 2)
				{
					return;		// culled
				}

				if(r == 1)
				{
					planeBits &= ~(1 << i);	// all descendants will also be in front
				}
			}
		}
	}

	if(node->contents != -1)
	{
		// leaf node, so add mark surfaces
		int             c;
		bspSurface_t   *surf, **mark;

		// add the individual surfaces
		mark = node->markSurfaces;
		c = node->numMarkSurfaces;
		while(c--)
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_AddInteractionSurface(surf, light);
			mark++;
		}
		return;
	}

	// node is just a decision point, so go down both sides
	// since we don't care about sort orders, just go positive to negative
	r = BoxOnPlaneSide(light->worldBounds[0], light->worldBounds[1], node->plane);

	switch (r)
	{
		case 1:
			R_RecursiveInteractionNode(node->children[0], light, planeBits);
			break;

		case 2:
			R_RecursiveInteractionNode(node->children[1], light, planeBits);
			break;

		case 3:
		default:
			// recurse down the children, front side first
			R_RecursiveInteractionNode(node->children[0], light, planeBits);
			R_RecursiveInteractionNode(node->children[1], light, planeBits);
			break;
	}
}


/*
===============
R_PointInLeaf
===============
*/
static bspNode_t *R_PointInLeaf(const vec3_t p)
{
	bspNode_t      *node;
	float           d;
	cplane_t       *plane;

	if(!tr.world)
	{
		ri.Error(ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while(1)
	{
		if(node->contents != -1)
		{
			break;
		}
		plane = node->plane;
		d = DotProduct(p, plane->normal) - plane->dist;
		if(d > 0)
		{
			node = node->children[0];
		}
		else
		{
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS(int cluster)
{
	if(!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
	{
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
qboolean R_inPVS(const vec3_t p1, const vec3_t p2)
{
	bspNode_t      *leaf;
	byte           *vis;

	leaf = R_PointInLeaf(p1);
	vis = ri.CM_ClusterPVS(leaf->cluster);
	leaf = R_PointInLeaf(p2);

	if(!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
	{
		return qfalse;
	}
	return qtrue;
}

/*
=================
BSPSurfaceCompare
compare function for qsort()
=================
*/
static int BSPSurfaceCompare(const void *a, const void *b)
{
	bspSurface_t   *aa, *bb;

	aa = *(bspSurface_t **) a;
	bb = *(bspSurface_t **) b;

	// shader first
	if(aa->shader < bb->shader)
		return -1;

	else if(aa->shader > bb->shader)
		return 1;

	// by lightmap
	if(aa->lightmapNum < bb->lightmapNum)
		return -1;

	else if(aa->lightmapNum > bb->lightmapNum)
		return 1;

	return 0;
}

/*
===============
R_UpdateClusterSurfaces()
===============
*/
static void R_UpdateClusterSurfaces()
{
	int             i, k, l;

	int             numVerts;
	int             numTriangles;

//  static glIndex_t indexes[MAX_MAP_DRAW_INDEXES];
//  static byte     indexes[MAX_MAP_DRAW_INDEXES * sizeof(glIndex_t)];
	glIndex_t      *indexes;
	int             indexesSize;

	shader_t       *shader, *oldShader;
	int             lightmapNum, oldLightmapNum;

	int             numSurfaces;
	bspSurface_t   *surface, *surface2;
	bspSurface_t  **surfacesSorted;

	bspCluster_t   *cluster;

	srfVBOMesh_t   *vboSurf;
	IBO_t          *ibo;

	vec3_t          bounds[2];

	if(tr.visClusters[tr.visIndex] < 0 || tr.visClusters[tr.visIndex] >= tr.world->numClusters)
	{
		// Tr3B: this is not a bug, the super cluster is the last one in the array
		cluster = &tr.world->clusters[tr.world->numClusters];
	}
	else
	{
		cluster = &tr.world->clusters[tr.visClusters[tr.visIndex]];
	}

	tr.world->numClusterVBOSurfaces[tr.visIndex] = 0;

	// count number of static cluster surfaces
	numSurfaces = 0;
	for(k = 0; k < cluster->numMarkSurfaces; k++)
	{
		surface = cluster->markSurfaces[k];
		shader = surface->shader;

		if(shader->isSky)
			continue;

		if(shader->isPortal)
			continue;

		if(ShaderRequiresCPUDeforms(shader))
			continue;

		numSurfaces++;
	}

	if(!numSurfaces)
		return;

	// build interaction caches list
	surfacesSorted = ri.Hunk_AllocateTempMemory(numSurfaces * sizeof(surfacesSorted[0]));

	numSurfaces = 0;
	for(k = 0; k < cluster->numMarkSurfaces; k++)
	{
		surface = cluster->markSurfaces[k];
		shader = surface->shader;

		if(shader->isSky)
			continue;

		if(shader->isPortal)
			continue;

		if(ShaderRequiresCPUDeforms(shader))
			continue;

		surfacesSorted[numSurfaces] = surface;
		numSurfaces++;
	}

	// sort surfaces by shader
	qsort(surfacesSorted, numSurfaces, sizeof(surfacesSorted), BSPSurfaceCompare);

	shader = oldShader = NULL;
	lightmapNum = oldLightmapNum = -1;

	for(k = 0; k < numSurfaces; k++)
	{
		surface = surfacesSorted[k];
		shader = surface->shader;
		lightmapNum = surface->lightmapNum;

		if(shader != oldShader || (r_precomputedLighting->integer ? lightmapNum != oldLightmapNum : 0))
		{
			oldShader = shader;
			oldLightmapNum = lightmapNum;

			// count vertices and indices
			numVerts = 0;
			numTriangles = 0;

			for(l = k; l < numSurfaces; l++)
			{
				surface2 = surfacesSorted[l];

				if(surface2->shader != shader || surface2->lightmapNum != lightmapNum)
					continue;

				if(*surface2->data == SF_FACE)
				{
					srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface2->data;

					if(!r_mergeClusterFaces->integer)
						continue;

					if(face->numVerts)
						numVerts += face->numVerts;

					if(face->numTriangles)
						numTriangles += face->numTriangles;
				}
				else if(*surface2->data == SF_GRID)
				{
					srfGridMesh_t  *grid = (srfGridMesh_t *) surface2->data;

					if(!r_mergeClusterCurves->integer)
						continue;

					if(grid->numVerts)
						numVerts += grid->numVerts;

					if(grid->numTriangles)
						numTriangles += grid->numTriangles;
				}
				else if(*surface2->data == SF_TRIANGLES)
				{
					srfTriangles_t *tri = (srfTriangles_t *) surface2->data;

					if(!r_mergeClusterTriangles->integer)
						continue;

					if(tri->numVerts)
						numVerts += tri->numVerts;

					if(tri->numTriangles)
						numTriangles += tri->numTriangles;
				}
			}

			if(!numVerts || !numTriangles)
				continue;

			ClearBounds(bounds[0], bounds[1]);

			// build triangle indices
			indexesSize = numTriangles * 3 * sizeof(glIndex_t);
			indexes = ri.Hunk_AllocateTempMemory(indexesSize);

			numTriangles = 0;
			for(l = k; l < numSurfaces; l++)
			{
				surface2 = surfacesSorted[l];

				if(surface2->shader != shader || surface2->lightmapNum != lightmapNum)
					continue;

				// set up triangle indices
				if(*surface2->data == SF_FACE)
				{
					srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface2->data;

					if(!r_mergeClusterFaces->integer)
						continue;

					if(srf->numTriangles)
					{
						srfTriangle_t  *tri;

						for(i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
					}
				}
				else if(*surface2->data == SF_GRID)
				{
					srfGridMesh_t  *srf = (srfGridMesh_t *) surface2->data;

					if(!r_mergeClusterCurves->integer)
						continue;

					if(srf->numTriangles)
					{
						srfTriangle_t  *tri;

						for(i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->meshBounds[0], srf->meshBounds[1]);
					}
				}
				else if(*surface2->data == SF_TRIANGLES)
				{
					srfTriangles_t *srf = (srfTriangles_t *) surface2->data;

					if(!r_mergeClusterTriangles->integer)
						continue;

					if(srf->numTriangles)
					{
						srfTriangle_t  *tri;

						for(i = 0, tri = tr.world->triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++)
						{
							indexes[numTriangles * 3 + i * 3 + 0] = tri->indexes[0];
							indexes[numTriangles * 3 + i * 3 + 1] = tri->indexes[1];
							indexes[numTriangles * 3 + i * 3 + 2] = tri->indexes[2];
						}

						numTriangles += srf->numTriangles;
						BoundsAdd(bounds[0], bounds[1], srf->bounds[0], srf->bounds[1]);
					}
				}
			}

			if(tr.world->numClusterVBOSurfaces[tr.visIndex] < tr.world->clusterVBOSurfaces[tr.visIndex].currentElements)
			{
				vboSurf =
					(srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[tr.visIndex],
														 tr.world->numClusterVBOSurfaces[tr.visIndex]);
				ibo = vboSurf->ibo;

				/*
				   if(ibo->indexesVBO)
				   {
				   qglDeleteBuffersARB(1, &ibo->indexesVBO);
				   ibo->indexesVBO = 0;
				   }
				 */

				//Com_Dealloc(ibo);
				//Com_Dealloc(vboSurf);
			}
			else
			{
				vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
				vboSurf->surfaceType = SF_VBO_MESH;

				vboSurf->vbo = tr.world->vbo;
				vboSurf->ibo = ibo = ri.Hunk_Alloc(sizeof(*ibo), h_low);

#if defined(USE_D3D10)
				// TODO
#else
				qglGenBuffersARB(1, &ibo->indexesVBO);
#endif

				Com_AddToGrowList(&tr.world->clusterVBOSurfaces[tr.visIndex], vboSurf);
			}

			//ri.Printf(PRINT_ALL, "creating VBO cluster surface for shader '%s'\n", shader->name);

			// update surface properties
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts = numVerts;

			vboSurf->shader = shader;
			vboSurf->lightmapNum = lightmapNum;

			VectorCopy(bounds[0], vboSurf->bounds[0]);
			VectorCopy(bounds[1], vboSurf->bounds[1]);

			// make sure the render thread is stopped
			R_SyncRenderThread();

			// update IBO
			Q_strncpyz(ibo->name,
					   va("staticWorldMesh_IBO_visIndex%i_surface%i", tr.visIndex, tr.world->numClusterVBOSurfaces[tr.visIndex]),
					   sizeof(ibo->name));
			ibo->indexesSize = indexesSize;

			R_BindIBO(ibo);
#if defined(USE_D3D10)
		// TODO
#else
			qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, GL_DYNAMIC_DRAW_ARB);
#endif
			R_BindNullIBO();

			//GL_CheckErrors();

			ri.Hunk_FreeTempMemory(indexes);

			tr.world->numClusterVBOSurfaces[tr.visIndex]++;
		}
	}

	ri.Hunk_FreeTempMemory(surfacesSorted);

	if(r_showcluster->modified || r_showcluster->integer)
	{
		r_showcluster->modified = qfalse;
		if(r_showcluster->integer)
		{
			ri.Printf(PRINT_ALL, "  surfaces:%i\n", tr.world->numClusterVBOSurfaces[tr.visIndex]);
		}
	}
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves(void)
{
	const byte     *vis;
	bspNode_t      *leaf, *parent;
	int             i;
	int             cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if(r_lockpvs->integer)// || r_dynamicBspOcclusionCulling->integer)
	{
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	for(i = 0; i < MAX_VISCOUNTS; i++)
	{
		if(tr.visClusters[i] == cluster)
		{
			//tr.visIndex = i;
			break;
		}
	}
	// if r_showcluster was just turned on, remark everything
	if(i != MAX_VISCOUNTS && !tr.refdef.areamaskModified && !r_showcluster->modified)// && !r_dynamicBspOcclusionCulling->modified)
	{
		if(tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer)
		{
			ri.Printf(PRINT_ALL, "found cluster:%i  area:%i  index:%i\n", cluster, leaf->area, i);
		}
		tr.visIndex = i;
		return;
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if(r_showcluster->modified || r_showcluster->integer)
	{
		//r_showcluster->modified = qfalse;
		if(r_showcluster->integer)
		{
			ri.Printf(PRINT_ALL, "update cluster:%i  area:%i  index:%i", cluster, leaf->area, tr.visIndex);
		}
	}

	/*
	if(r_dynamicBspOcclusionCulling->modified)
	{
		r_dynamicBspOcclusionCulling->modified = qfalse;
	}
	*/

	if(!r_dynamicBspOcclusionCulling->integer)
	{
		R_UpdateClusterSurfaces();
	}

	if(r_novis->integer || tr.visClusters[tr.visIndex] == -1)
	{
		for(i = 0; i < tr.world->numnodes; i++)
		{
			if(tr.world->nodes[i].contents != CONTENTS_SOLID)
			{
				tr.world->nodes[i].visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			}
		}
		return;
	}

	vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	for(i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++)
	{
		if(tr.world->vis)
		{
			cluster = leaf->cluster;

			if(cluster >= 0 && cluster < tr.world->numClusters)
			{
				// check general pvs
				if(!(vis[cluster >> 3] & (1 << (cluster & 7))))
				{
					continue;
				}
			}
		}

		// check for door connection
		if((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7))))
		{
			continue;			// not visible
		}

		parent = leaf;
		do
		{
			if(parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex])
				break;
			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent = parent->parent;
		} while(parent);
	}
}


static void DrawLeaf(bspNode_t * node)
{
	// leaf node, so add mark surfaces
	int             c;
	bspSurface_t   *surf, **mark;

	tr.pc.c_leafs++;

	// add to z buffer bounds
	if(node->mins[0] < tr.viewParms.visBounds[0][0])
	{
		tr.viewParms.visBounds[0][0] = node->mins[0];
	}
	if(node->mins[1] < tr.viewParms.visBounds[0][1])
	{
		tr.viewParms.visBounds[0][1] = node->mins[1];
	}
	if(node->mins[2] < tr.viewParms.visBounds[0][2])
	{
		tr.viewParms.visBounds[0][2] = node->mins[2];
	}

	if(node->maxs[0] > tr.viewParms.visBounds[1][0])
	{
		tr.viewParms.visBounds[1][0] = node->maxs[0];
	}
	if(node->maxs[1] > tr.viewParms.visBounds[1][1])
	{
		tr.viewParms.visBounds[1][1] = node->maxs[1];
	}
	if(node->maxs[2] > tr.viewParms.visBounds[1][2])
	{
		tr.viewParms.visBounds[1][2] = node->maxs[2];
	}

	// add the individual surfaces
	mark = node->markSurfaces;
	c = node->numMarkSurfaces;
	while(c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface(surf);
		mark++;
	}
}

static qboolean InsideViewFrustum(bspNode_t * node, int planeBits)
{
	if(!r_nocull->integer)
	{
		int             i;
		int             r;

		for(i = 0; i < FRUSTUM_PLANES; i++)
		{
			if(planeBits & (1 << i))
			{
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
				if(r == 2)
				{
					return qfalse;	// culled
				}
				if(r == 1)
				{
					planeBits &= ~(1 << i);	// all descendants will also be in front
				}
			}
		}
	}

	return qtrue;
}




static void DrawNode_r(bspNode_t * node, int planeBits)
{
	do
	{
		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?
		if(!r_nocull->integer)
		{
			int             i;
			int             r;

			for(i = 0; i < FRUSTUM_PLANES; i++)
			{
				if(planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
					if(r == 2)
					{
						return;	// culled
					}
					if(r == 1)
					{
						planeBits &= ~(1 << i);	// all descendants will also be in front
					}
				}
			}
		}

		if(node->contents != -1 && !(node->contents & CONTENTS_TRANSLUCENT))
		{
#if defined(USE_D3D10)
			//TODO
#else
			R_BindVBO(node->volumeVBO);
			R_BindIBO(node->volumeIBO);

			GL_VertexAttribsState(ATTR_POSITION);

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes = node->volumeIndexes;

			Tess_DrawElements();

			tess.numIndexes = 0;
			tess.numVertexes = 0;
#endif
			break;
		}

		// recurse down the children, front side first
		DrawNode_r(node->children[0], planeBits);

		// tail recurse
		node = node->children[1];
	} while(1);
}





static void IssueOcclusionQuery(link_t * queue, bspNode_t * node, qboolean resetMultiQueryLink)
{
	GLimp_LogComment("--- IssueOcclusionQuery ---\n");

	//ri.Printf(PRINT_ALL, "--- IssueOcclusionQuery(%i) ---\n", node - tr.world->nodes);

	EnQueue(queue, node);

	// tell GetOcclusionQueryResult that this is not a multi query
	if(resetMultiQueryLink)
	{
		QueueInit(&node->multiQuery);
	}

#if !defined(USE_D3D10)
	//Tess_EndBegin();

	GL_CheckErrors();

#if 0
	if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
	{
		ri.Error(ERR_FATAL, "IssueOcclusionQuery: node %i has already an occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	// begin the occlusion query
	qglBeginQueryARB(GL_SAMPLES_PASSED, node->occlusionQueryObjects[tr.viewCount]);

	GL_CheckErrors();

	R_BindVBO(node->volumeVBO);
	R_BindIBO(node->volumeIBO);

	GL_VertexAttribsState(ATTR_POSITION);

	tess.numVertexes = node->volumeVerts;
	tess.numIndexes = node->volumeIndexes;

	Tess_DrawElements();

	// end the query
	qglEndQueryARB(GL_SAMPLES_PASSED);

#if 1
	if(!qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
	{
		ri.Error(ERR_FATAL, "IssueOcclusionQuery: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	node->occlusionQueryNumbers[tr.viewCount] = tr.pc.c_occlusionQueries;
	tr.pc.c_occlusionQueries++;

	tess.numIndexes = 0;
	tess.numVertexes = 0;

	GL_CheckErrors();
#endif
}

static void IssueMultiOcclusionQueries(link_t * multiQueue, link_t * individualQueue)
{
	bspNode_t *node;
	bspNode_t *multiQueryNode;
	link_t *l;

	GLimp_LogComment("--- IssueMultiOcclusionQueries ---\n");

#if 0
	ri.Printf(PRINT_ALL, "IssueMultiOcclusionQueries(");
	for(l = multiQueue->prev; l != multiQueue; l = l->prev)
	{
		node = (bspNode_t *) l->data;

		ri.Printf(PRINT_ALL, "%i, ", node - tr.world->nodes);
	}
	ri.Printf(PRINT_ALL, ")\n");
#endif

	if(QueueEmpty(multiQueue))
		return;

	multiQueryNode = (bspNode_t *) QueueFront(multiQueue)->data;

	// begin the occlusion query
#if defined(USE_D3D10)
	// TODO
#else
	GL_CheckErrors();

#if 0
	if(!qglIsQueryARB(multiQueryNode->occlusionQueryObjects[tr.viewCount]))
	{
		ri.Error(ERR_FATAL, "IssueMultiOcclusionQueries: node %i has already occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, tr.viewCount, multiQueryNode->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	qglBeginQueryARB(GL_SAMPLES_PASSED, multiQueryNode->occlusionQueryObjects[tr.viewCount]);

	GL_CheckErrors();
#endif

	//ri.Printf(PRINT_ALL, "rendering nodes:[");
	for(l = multiQueue->prev; l != multiQueue; l = l->prev)
	//l = multiQueue->prev;
	{
		node = (bspNode_t *) l->data;

		//ri.Printf(PRINT_ALL, "%i, ", node - tr.world->nodes);

#if defined(USE_D3D10)
		// TODO
#else
		//Tess_EndBegin();

		R_BindVBO(node->volumeVBO);
		R_BindIBO(node->volumeIBO);

		GL_VertexAttribsState(ATTR_POSITION);

		tess.numVertexes = node->volumeVerts;
		tess.numIndexes = node->volumeIndexes;

		Tess_DrawElements();

		tess.numIndexes = 0;
		tess.numVertexes = 0;
#endif
	}
	//ri.Printf(PRINT_ALL, "]\n");

	multiQueryNode->occlusionQueryNumbers[tr.viewCount] = tr.pc.c_occlusionQueries;
	tr.pc.c_occlusionQueries++;
	tr.pc.c_occlusionQueriesMulti++;

	// end the query
#if defined(USE_D3D10)
	// TODO
#else
	qglEndQueryARB(GL_SAMPLES_PASSED);

	GL_CheckErrors();
#endif

#if 0
	if(!qglIsQueryARB(multiQueryNode->occlusionQueryObjects[tr.viewCount]))
	{
		ri.Error(ERR_FATAL, "IssueMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, tr.viewCount, multiQueryNode->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	// move queue to node->multiQuery queue
	QueueInit(&multiQueryNode->multiQuery);
	DeQueue(multiQueue);
	while(!QueueEmpty(multiQueue))
	{
		node = (bspNode_t *) DeQueue(multiQueue);
		EnQueue(&multiQueryNode->multiQuery, node);
	}

	EnQueue(individualQueue, multiQueryNode);

	//ri.Printf(PRINT_ALL, "--- IssueMultiOcclusionQueries end ---\n");
}

static qboolean ResultAvailable(bspNode_t *node)
{
#if defined(USE_D3D10)
	// TODO
#else
	GLint			available;

	qglFinish();

	available = 0;
	//if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
	{
		qglGetQueryObjectivARB(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
		GL_CheckErrors();
	}

	return !!available;
#endif
}

static void GetOcclusionQueryResult(bspNode_t *node)
{
	link_t			*l, *sentinel;
	int			     ocSamples;

#if defined(USE_D3D10)
	// TODO
#else
	GLint			available;

	GLimp_LogComment("--- GetOcclusionQueryResult ---\n");

	qglFinish();

#if 0
	if(!qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
	{
		ri.Error(ERR_FATAL, "GetOcclusionQueryResult: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, tr.viewCount, node->occlusionQueryObjects[tr.viewCount]);
	}
#endif

	available = 0;
	while(!available)
	{
		//if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
		{
			qglGetQueryObjectivARB(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
			//GL_CheckErrors();
		}
	}

	qglGetQueryObjectivARB(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT, &ocSamples);

	//ri.Printf(PRINT_ALL, "GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

	GL_CheckErrors();
#endif

	node->occlusionQuerySamples[tr.viewCount] = ocSamples;
	node->lastQueried[tr.viewCount] = tr.frameCount;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &node->multiQuery;
	for(l = sentinel->prev; l != sentinel; l = l->prev)
	{
		node = (bspNode_t *) l->data;

		node->occlusionQuerySamples[tr.viewCount] = ocSamples;
		node->lastQueried[tr.viewCount] = tr.frameCount;
	}
}

static void PullUpVisibility(bspNode_t * node)
{
	bspNode_t      *parent;

	parent = node;
	do
	{
		if(parent->visible)
			break;

		parent->visible[tr.viewCount] = qtrue;
		parent->lastVisited[tr.viewCount] = tr.frameCount;
		parent = parent->parent;
	} while(parent);
}

static void PushNode(link_t * traversalStack, bspNode_t * node)
{
	if(node->contents != -1)
	{
		DrawLeaf(node);
	}
	else
	{
		//float			d1, d2;
		cplane_t       *splitPlane;

		splitPlane = node->plane;

		//d1 = DistanceSquared(tr.viewParms.orientation.origin, node->children[0]->origin);
		//d2 = DistanceSquared(tr.viewParms.orientation.origin, node->children[1]->origin);

		//if(d1 <= d2)
#if 0
		if(DotProduct(splitPlane->normal, tr.viewParms.orientation.axis[0]) <= 0)
		{
			StackPush(traversalStack, node->children[0]);
			StackPush(traversalStack, node->children[1]);

			ri.Printf(PRINT_ALL, "--> %i\n", node->children[0] - tr.world->nodes);
			ri.Printf(PRINT_ALL, "--> %i\n", node->children[1] - tr.world->nodes);
		}
		else
#endif
		{
			StackPush(traversalStack, node->children[0]);
			StackPush(traversalStack, node->children[1]);

			//ri.Printf(PRINT_ALL, "--> %i\n", node->children[1] - tr.world->nodes);
			//ri.Printf(PRINT_ALL, "--> %i\n", node->children[0] - tr.world->nodes);
		}
	}
}

static void BuildNodeTraversalStackPost_r(bspNode_t * node)
{
	do
	{

#if 1
		if((tr.frameCount != node->lastVisited[tr.viewCount]))// > r_chcMaxVisibleFrames->integer)
		{
			return;
		}
#endif

#if 0
		if((tr.frameCount - node->lastQueried[tr.viewCount]) <= r_chcMaxVisibleFrames->integer)
#else
		// if r_chcMaxVisibleFrames 10 then range from 5 to 10
		if((tr.frameCount - node->lastQueried[tr.viewCount]) <= Q_min((int)ceil((r_chcMaxVisibleFrames->value * 0.5f) + (r_chcMaxVisibleFrames->value * 0.5f) * random()), r_chcMaxVisibleFrames->integer))
#endif
		{
			node->visible[tr.viewCount] = node->occlusionQuerySamples[tr.viewCount] > r_chcVisibilityThreshold->integer;
		}
		else
		{
			node->visible[tr.viewCount] = qfalse;
		}

		if(tr.frameCount == node->lastVisited[tr.viewCount])
		{
			InsertLink(&node->visChain, &tr.traversalStack);
		}

		if(node->contents != -1)
		{
			if(node->visible[tr.viewCount])
			{
				PullUpVisibility(node);
			}
			break;
		}

		// recurse down the children, front side first
		BuildNodeTraversalStackPost_r(node->children[0]);

		// tail recurse
		node = node->children[1];
	} while(1);
}

static void R_CoherentHierachicalCulling()
{
	bspNode_t      *node;
	bspNode_t      *multiQueryNode;

	link_t			traversalStack;
	link_t			occlusionQueryQueue;
	link_t			visibleQueue; // CHC++
	link_t			invisibleQueue; // CHC++
	link_t			renderQueue;
	int             startTime = 0, endTime = 0;

	qboolean		wasVisible;
	qboolean		needsQuery;

	//ri.Cvar_Set("r_logFile", "1");

#if defined(USE_D3D10)
	// TODO
#else

	GLimp_LogComment("--- R_CoherentHierachicalCulling++ ---\n");

	//ri.Printf(PRINT_ALL, "--- R_CHC++ begin ---\n");

	//ri.Printf(PRINT_ALL, "tr.viewCount = %i, tr.viewCountNoReset = %i\n", tr.viewCount, tr.viewCountNoReset);

	if(r_speeds->integer)
	{
		qglFinish();
		startTime = ri.Milliseconds();
	}

	if(DS_STANDARD_ENABLED())
	{
		R_BindFBO(tr.deferredRenderFBO);
	}
	else if(DS_PREPASS_LIGHTING_ENABLED())
	{
#if defined(OFFSCREEN_PREPASS_LIGHTING)
		R_BindFBO(tr.deferredRenderFBO);
#else
		R_BindNullFBO();
#endif
	}
	else if(HDR_ENABLED())
	{
		R_BindFBO(tr.deferredRenderFBO);
	}
	else
	{
		R_BindNullFBO();
	}

	GL_BindProgram(&tr.genericSingleShader);
	GL_Cull(CT_TWO_SIDED);

	GL_LoadProjectionMatrix(tr.viewParms.projectionMatrix);

	GL_Viewport(tr.viewParms.viewportX, tr.viewParms.viewportY,
				tr.viewParms.viewportWidth, tr.viewParms.viewportHeight);

	GL_Scissor(tr.viewParms.viewportX, tr.viewParms.viewportY,
			   tr.viewParms.viewportWidth, tr.viewParms.viewportHeight);

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
	GL_LoadModelViewMatrix(tr.orientation.modelViewMatrix);
	GLSL_SetUniform_ModelViewProjectionMatrix(&tr.genericSingleShader, glState.modelViewProjectionMatrix[glState.stackIndex]);

	// bind u_ColorMap
	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
	GLSL_SetUniform_ColorTextureMatrix(&tr.genericSingleShader, matrixIdentity);

#if 0
	GL_ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	qglClear(GL_DEPTH_BUFFER_BIT);

	GL_State(GLS_COLORMASK_BITS | GLS_DEPTHMASK_TRUE);

	// draw BSP leaf volumes to depth
	DrawNode_r(&tr.world->nodes[0], FRUSTUM_CLIPALL);

	GL_State(GLS_COLORMASK_BITS);
#else
	// use the depth buffer of the previous frame for occlusion culling
	GL_State(GLS_COLORMASK_BITS);
#endif

	ClearLink(&tr.traversalStack);
	QueueInit(&tr.occlusionQueryQueue);
	ClearLink(&tr.occlusionQueryList);

	ClearLink(&traversalStack);
	QueueInit(&occlusionQueryQueue);
	QueueInit(&visibleQueue);
	QueueInit(&invisibleQueue);
	QueueInit(&renderQueue);

	StackPush(&traversalStack, &tr.world->nodes[0]);

	CHCLoop:

	while(!StackEmpty(&traversalStack) || !QueueEmpty(&occlusionQueryQueue) || !QueueEmpty(&invisibleQueue))
	{
		//ri.Printf(PRINT_ALL, "--- (%i, %i, %i)\n", !StackEmpty(&traversalStack), !QueueEmpty(&occlusionQueryQueue), !QueueEmpty(&invisibleQueue));

		//--PART 1: process finished occlusion queries
		while(!QueueEmpty(&occlusionQueryQueue) && (ResultAvailable(QueueFront(&occlusionQueryQueue)->data) || StackEmpty(&traversalStack)))
		{
			if(ResultAvailable(QueueFront(&occlusionQueryQueue)->data))
			{
				node = (bspNode_t *) DeQueue(&occlusionQueryQueue);

				// wait if result not available
				GetOcclusionQueryResult(node);

				if(node->occlusionQuerySamples[tr.viewCount] > r_chcVisibilityThreshold->integer)
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if(!QueueEmpty(&node->multiQuery))
					{
						multiQueryNode = node;

						IssueOcclusionQuery(&occlusionQueryQueue, multiQueryNode, qfalse);

						while(!QueueEmpty(&multiQueryNode->multiQuery))
						{
							node = (bspNode_t *) DeQueue(&multiQueryNode->multiQuery);

							IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
						}
					}
					else
					{
						if(r_logFile->integer)
						{
							// don't just call LogComment, or we will get
							// a call to va() every frame!
							GLimp_LogComment(va("node %i visible\n", node - tr.world->nodes));
						}

						PullUpVisibility(node);
						PushNode(&traversalStack, node);
					}
				}
				else
				{
					if(!QueueEmpty(&node->multiQuery))
					{
						node->visible[tr.viewCount] = qfalse;

						multiQueryNode = node;
						while(!QueueEmpty(&multiQueryNode->multiQuery))
						{
							node = (bspNode_t *) DeQueue(&multiQueryNode->multiQuery);

							node->visible[tr.viewCount] = qfalse;

							tr.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						node->visible[tr.viewCount] = qfalse;
					}
				}
			}
			else
			{
				if(!QueueEmpty(&visibleQueue))
				{
					node = (bspNode_t *) DeQueue(&visibleQueue);

					IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
				}
			}
		}

		//--PART 2: hierarchical traversal
		if(!StackEmpty(&traversalStack))
		{
			node = (bspNode_t *) StackPop(&traversalStack);

			//ri.Printf(PRINT_ALL, "<-- %i\n", node - tr.world->nodes);

#if 1
			// if the node wasn't marked as potentially visible, exit
			if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
				continue;
#endif

			// don't waste time dealing with empty leaves
			if(node->contents != -1 && !node->numMarkSurfaces)
				continue;

			if(!InsideViewFrustum(node, FRUSTUM_CLIPALL))
				continue;


			// identify previously visible nodes
			wasVisible = node->visible[tr.viewCount] && ((tr.frameCount - node->lastVisited[tr.viewCount]) <= r_chcMaxVisibleFrames->integer);

			// identify nodes that we cannot skip queries for

			// reset node's visibility classification
			//if(BoundsIntersectPoint(node->mins, node->maxs, tr.viewParms.orientation.origin))
			if(BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][FRUSTUM_NEAR]) == 3)
			{
				// node clips near plane so avoid the occlusion query test
				node->occlusionQuerySamples[tr.viewCount] = r_chcVisibilityThreshold->integer + 1;
				node->lastQueried[tr.viewCount] = tr.frameCount;
				node->visible[tr.viewCount] = qtrue;

				needsQuery = qfalse;
			}
			else if(r_dynamicBspOcclusionCulling->integer == 2 && node->contents == -1)
			{
				// setting all BSP nodes to visible will traverse to all leaves
				// this has the advantage that we can group leaf queries which will save really many occlusion queries
				node->occlusionQuerySamples[tr.viewCount] = r_chcVisibilityThreshold->integer + 1;
				node->lastQueried[tr.viewCount] = tr.frameCount;
				node->visible[tr.viewCount] = qtrue;

				needsQuery = qfalse;
			}
			else if(r_dynamicBspOcclusionCulling->integer == 1 && node->contents != -1)
			{
				// NOTE: this is the fastest dynamic occlusion culling path

				// only very few leaves are invisible if we don't traverse through all bsp nodes
				// so testing these leaves just causes additional occlusion queries which can be avoided
				// by setting all reached leaves to visible
				node->occlusionQuerySamples[tr.viewCount] = r_chcVisibilityThreshold->integer + 1;
				node->lastQueried[tr.viewCount] = tr.frameCount;
				node->visible[tr.viewCount] = qtrue;

				needsQuery = qfalse;
			}
			else
			{
				// CHC default
				needsQuery = !wasVisible || (node->contents != -1);
			}

			// update node's visited flag
			node->lastVisited[tr.viewCount] = tr.frameCount;

			// optimization
#if 0
			if((node->contents != -1) && node->sameAABBAsParent)
			{
				node->visible[tr.viewCount] = qtrue;
				wasVisible = qtrue;
			}
#endif


			if(needsQuery)
			{
#if 0
				IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
#else
				EnQueue(&invisibleQueue, node);

				if(QueueSize(&invisibleQueue) >= r_chcMaxPrevInvisNodesBatchSize->integer)
					IssueMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);
#endif
			}
			else
			{
#if 0
				if((node->contents != -1)) // && QueryReasonable(node))
				{
					EnQueue(&visibleQueue, node);
				}
#endif

				// always traverse a node if it was visible
				PushNode(&traversalStack, node);
			}
		}
		else
		{
			if(!QueueEmpty(&invisibleQueue))
			{
				// remaining previously visible node queries
				IssueMultiOcclusionQueries(&invisibleQueue, &occlusionQueryQueue);

				//ri.Printf(PRINT_ALL, "occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
			}
		}

		//ri.Printf(PRINT_ALL, "--- (%i, %i, %i)\n", !StackEmpty(&traversalStack), !QueueEmpty(&occlusionQueryQueue), !QueueEmpty(&invisibleQueue));
	}

	if(!QueueEmpty(&visibleQueue))
	{
		while(!QueueEmpty(&visibleQueue))
		{
			node = (bspNode_t *) DeQueue(&visibleQueue);

			IssueOcclusionQuery(&occlusionQueryQueue, node, qtrue);
		}

		goto CHCLoop;
	}

	ClearLink(&tr.traversalStack);
	BuildNodeTraversalStackPost_r(&tr.world->nodes[0]);

	R_BindNullFBO();

	// reenable color buffer and depth buffer writes
	GL_State(GLS_DEFAULT);

	GL_CheckErrors();

	//ri.Printf(PRINT_ALL, "--- R_CHC++ end ---\n");

	if(r_speeds->integer)
	{
		qglFinish();
		endTime = ri.Milliseconds();
		tr.pc.c_CHCTime = endTime - startTime;
	}
#endif
}






static void R_RecursiveChainWorldNode(bspNode_t * node, int planeBits)
{
	do
	{
		qboolean wasVisible;
		qboolean needsQuery;
		qboolean intersect;

		// if the node wasn't marked as potentially visible, exit
		if(node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex])
		{
			return;
		}

		// don't waste time dealing empty leaves
		if(node->contents != -1 && !node->numMarkSurfaces)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?
		if(!r_nocull->integer)
		{
			int             i;
			int             r;

			for(i = 0; i < FRUSTUM_PLANES; i++)
			{
				if(planeBits & (1 << i))
				{
					r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustums[0][i]);
					if(r == 2)
					{
						return;	// culled
					}
					if(r == 1)
					{
						planeBits &= ~(1 << i);	// all descendants will also be in front
					}
				}
			}
		}

		InsertLink(&node->visChain, &tr.traversalStack);

		// identify previously visible nodes
#if 1
		if(r_dynamicBspOcclusionCulling->integer == 2)
		{
			if(node->contents != -1)
				wasVisible = node->visible[tr.viewCount] && (node->lastVisited[tr.viewCount] == tr.frameCount -1);
			else
				wasVisible = qtrue;//node->visible;
		}
		else
		{
			wasVisible = node->visible[tr.viewCount] && (node->lastVisited[tr.viewCount] == tr.frameCount -1);
		}
#else
		wasVisible = node->visible[tr.viewCount] && (node->lastVisited[tr.viewCount] == tr.frameCount -1);
#endif

		// reset node's visibility classification
		intersect = BoundsIntersectPoint(node->mins, node->maxs, tr.viewParms.orientation.origin);
#if 1
		if(intersect)
		{
			node->visible[tr.viewCount] = qtrue;
			wasVisible = qtrue;
		}
		else
		{
			if(r_dynamicBspOcclusionCulling->integer == 2)
			{
				if(node->contents != -1)
					node->visible[tr.viewCount] = qfalse;
				else
					node->visible[tr.viewCount] = qtrue;
			}
			else
			{
				node->visible[tr.viewCount] = qfalse;
			}
		}
#else
		node->visible = qfalse;
#endif

		// update node's visited flag
		node->lastVisited[tr.viewCount] = tr.frameCount;

		// identify nodes that we cannot skip queries for
		needsQuery = (!wasVisible || (node->contents != -1)) && !intersect;
		//needsQuery = (node->contents != -1) && !intersect;
		//needsQuery = (!wasVisible || (node->contents != -1) || (node->lastVisited != tr.frameCount -1)) && !intersect;
		//needsQuery = (!wasVisible || (node->lastVisited != tr.frameCount -1)) && !intersect;
		//needsQuery = !wasVisible && (node->contents != -1) && (node->lastVisited != tr.frameCount -1) && !intersect;
		//needsQuery = !wasVisible && (node->contents != -1) && !intersect;

		// skip testing previously visible interior nodes
		if(needsQuery)
		{
			InsertLink(&node->occlusionQuery, &tr.occlusionQueryList);
			node->issueOcclusionQuery[tr.viewCount] = qtrue;
		}

		if(wasVisible)
		{
			if(node->contents != -1)
			{
				DrawLeaf(node);
				break;
			}
			else
			{
				//float			d1, d2;
				cplane_t       *splitPlane;

				splitPlane = node->plane;

				//d1 = DistanceSquared(tr.viewParms.orientation.origin, node->children[0]->origin);
				//d2 = DistanceSquared(tr.viewParms.orientation.origin, node->children[1]->origin);

				//if(d1 <= d2)
				if(DotProduct(splitPlane->normal, tr.viewParms.orientation.axis[0]) <= 0)
				{
					// recurse down the children, front side first
					R_RecursiveChainWorldNode(node->children[0], planeBits);

					// tail recurse
					node = node->children[1];
				}
				else
				{
					R_RecursiveChainWorldNode(node->children[1], planeBits);

					// tail recurse
					node = node->children[0];
				}
			}
		}
		else
		{
			break;
		}

	} while(1);
}

static void MarkInvisible(bspNode_t * node)
{
	do
	{
		// update node's visited flag
		node->lastVisited[tr.viewCount] = tr.frameCount;

		node->visible[tr.viewCount] = qfalse;

		if(node->contents != -1)
			break;

		// recurse down the children, front side first
		MarkInvisible(node->children[0]);

		// tail recurse
		node = node->children[1];

	} while(1);
}

static void R_CoherentHierachicalCulling2()
{
	bspNode_t      *node, *parent;
	link_t		   *l, *sentinel;

	// wait until all occlusion queries from the previous frame are ready to read
#if 0
#if defined(USE_D3D10)
	// TODO
#else
	int				ocCount;
	int             avCount;
	GLint           available;

	qglFinish();

	ocCount = 0;
	sentinel = &tr.occlusionQueryList;
	for(l = sentinel->next; l != sentinel; l = l->next)
	{
		node = (bspNode_t *) l->data;

		if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
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
				if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
				{
					qglGetQueryObjectivARB(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT_AVAILABLE_ARB, &available);
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

	// grab the samples
	for(l = sentinel->next; l != sentinel; l = l->next)
	{
		node = (bspNode_t *) l->data;

		if(qglIsQueryARB(node->occlusionQueryObjects[tr.viewCount]))
		{
			qglGetQueryObjectivARB(node->occlusionQueryObjects[tr.viewCount], GL_QUERY_RESULT, &node->occlusionQuerySamples[tr.viewCount]);
		}
	}

	GL_CheckErrors();

	//ri.Printf(PRINT_ALL, "done\n");

#endif // USE_D3D10

#endif

	sentinel = &tr.occlusionQueryList;
	for(l = sentinel->next; l != sentinel; l = l->next)
	{
		node = (bspNode_t *) l->data;
#if 1
		if(node->occlusionQuerySamples[tr.viewCount] <= 0)// && !BoundsIntersectPoint(node->mins, node->maxs, tr.viewParms.orientation.origin))
		{
			node->visible[tr.viewCount] = qfalse;
			//MarkInvisible(node);
			continue;
		}
#endif

#if 1
		// pull up visibility
		parent = node;
		do
		{
			if(parent->visible)
				break;

			parent->visible[tr.viewCount] = qtrue;
			parent->lastVisited[tr.viewCount] = tr.frameCount -1;

			parent = parent->parent;
		} while(parent);
#endif
	}

	ClearLink(&tr.traversalStack);
	ClearLink(&tr.occlusionQueryQueue);
	ClearLink(&tr.occlusionQueryList);

	R_RecursiveChainWorldNode(tr.world->nodes, FRUSTUM_CLIPALL);
}

/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces(void)
{
	if(!r_drawworld->integer)
	{
		return;
	}

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	// update the bsp nodes with the dynamic occlusion query results
	if(glConfig.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer)
	{
		R_CoherentHierachicalCulling();
	}
	else
	{
		ClearLink(&tr.traversalStack);
		ClearLink(&tr.occlusionQueryQueue);
		ClearLink(&tr.occlusionQueryList);

		// update visbounds and add surfaces that weren't cached with VBOs
		R_RecursiveWorldNode(tr.world->nodes, FRUSTUM_CLIPALL);
	}

	if(r_mergeClusterSurfaces->integer && !r_dynamicBspOcclusionCulling->integer)
	{
		int             j, i;
		srfVBOMesh_t   *srf;
		shader_t       *shader;
		cplane_t       *frust;
		int             r;

		for(j = 0; j < tr.world->numClusterVBOSurfaces[tr.visIndex]; j++)
		{
			srf = (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[tr.visIndex], j);
			shader = srf->shader;

			for(i = 0; i < FRUSTUM_PLANES; i++)
			{
				frust = &tr.viewParms.frustums[0][i];

				r = BoxOnPlaneSide(srf->bounds[0], srf->bounds[1], frust);

				if(r == 2)
				{
					// completely outside frustum
					continue;
				}
			}

			R_AddDrawSurf((void *)srf, shader, srf->lightmapNum);
		}
	}
}

/*
=============
R_AddWorldInteractions
=============
*/
void R_AddWorldInteractions(trRefLight_t * light)
{
	if(!r_drawworld->integer)
	{
		return;
	}

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	// perform frustum culling and add all the potentially visible surfaces
	tr.lightCount++;
	R_RecursiveInteractionNode(tr.world->nodes, light, FRUSTUM_CLIPALL);

	if(r_vboDynamicLighting->integer)
	{
		int             j;
		srfVBOMesh_t   *srf;
		shader_t       *shader;
		qboolean        intersects;
		interactionType_t iaType = IA_DEFAULT;
		byte            cubeSideBits = CUBESIDE_CLIPALL;

		for(j = 0; j < tr.world->numClusterVBOSurfaces[tr.visIndex]; j++)
		{
			srf = (srfVBOMesh_t *) Com_GrowListElement(&tr.world->clusterVBOSurfaces[tr.visIndex], j);
			shader = srf->shader;

			//  skip all surfaces that don't matter for lighting only pass
			if(shader->isSky || (!shader->interactLight && shader->noShadows))
				continue;

			intersects = qtrue;

			// do a quick AABB cull
			if(!BoundsIntersect(light->worldBounds[0], light->worldBounds[1], srf->bounds[0], srf->bounds[1]))
				intersects = qfalse;

			// FIXME? do a more expensive and precise light frustum cull
			if(!r_noLightFrustums->integer)
			{
				if(R_CullLightWorldBounds(light, srf->bounds) == CULL_OUT)
				{
					intersects = qfalse;
				}
			}

			// FIXME?
			if(r_cullShadowPyramidFaces->integer)
			{
				cubeSideBits = R_CalcLightCubeSideBits(light, srf->bounds);
			}

			if(intersects)
			{
				R_AddLightInteraction(light, (void *)srf, srf->shader, cubeSideBits, iaType);

				if(light->isStatic)
					tr.pc.c_slightSurfaces++;
				else
					tr.pc.c_dlightSurfaces++;
			}
			else
			{
				if(!light->isStatic)
					tr.pc.c_dlightSurfacesCulled++;
			}
		}
	}
}

/*
=============
R_AddPrecachedWorldInteractions
=============
*/
void R_AddPrecachedWorldInteractions(trRefLight_t * light)
{
	interactionType_t iaType = IA_DEFAULT;

	if(!r_drawworld->integer)
	{
		return;
	}

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL)
	{
		return;
	}

	if(!light->firstInteractionCache)
	{
		// this light has no interactions precached
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	if((r_vboShadows->integer || r_vboLighting->integer))// && light->l.rlType != RL_DIRECTIONAL)
	{
		interactionCache_t *iaCache;
		interactionVBO_t *iaVBO;
		srfVBOMesh_t   *srf;
		srfVBOShadowVolume_t *shadowSrf;
		shader_t       *shader;
		bspSurface_t   *surface;

		if(r_shadows->integer == 3)
		{
			for(iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
			{
				if(!iaVBO->vboShadowVolume)
					continue;

				shadowSrf = iaVBO->vboShadowVolume;

				R_AddLightInteraction(light, (void *)shadowSrf, tr.defaultShader, CUBESIDE_CLIPALL, IA_SHADOWONLY);
			}

			for(iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
			{
				if(!iaVBO->vboLightMesh)
					continue;

				srf = iaVBO->vboLightMesh;
				shader = iaVBO->shader;

				R_AddLightInteraction(light, (void *)srf, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY);
			}
		}
		else
		{
			// this can be shadow mapping or shadowless lighting
			for(iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
			{
				if(!iaVBO->vboLightMesh)
					continue;

				srf = iaVBO->vboLightMesh;
				shader = iaVBO->shader;

				switch (light->l.rlType)
				{
					case RL_OMNI:
						R_AddLightInteraction(light, (void *)srf, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY);
						break;

					case RL_DIRECTIONAL:
					case RL_PROJ:
						R_AddLightInteraction(light, (void *)srf, shader, CUBESIDE_CLIPALL, IA_LIGHTONLY);
						break;

					default:
						R_AddLightInteraction(light, (void *)srf, shader, CUBESIDE_CLIPALL, IA_DEFAULT);
						break;
				}
			}

			// add meshes for shadowmap generation if any
			for(iaVBO = light->firstInteractionVBO; iaVBO; iaVBO = iaVBO->next)
			{
				if(!iaVBO->vboShadowMesh)
					continue;

				srf = iaVBO->vboShadowMesh;
				shader = iaVBO->shader;

				R_AddLightInteraction(light, (void *)srf, shader, iaVBO->cubeSideBits, IA_SHADOWONLY);
			}
		}

		for(iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
		{
			if(iaCache->redundant)
				continue;

			if(iaCache->mergedIntoVBO)
				continue;

			surface = iaCache->surface;

			// Tr3B - this surface is maybe not in this view but it may still cast a shadow
			// into this view
			if(surface->viewCount != tr.viewCountNoReset)
			{
				if(r_shadows->integer <= 3 || light->l.noShadows)
					continue;
				else
					iaType = IA_SHADOWONLY;
			}
			else
			{
				iaType = iaCache->type;
			}

			R_AddLightInteraction(light, surface->data, surface->shader, iaCache->cubeSideBits, iaType);
		}
	}
	else
	{
		interactionCache_t *iaCache;
		bspSurface_t   *surface;

		for(iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
		{
			if(iaCache->redundant)
				continue;

			surface = iaCache->surface;

			// Tr3B - this surface is maybe not in this view but it may still cast a shadow
			// into this view
			if(surface->viewCount != tr.viewCountNoReset)
			{
				if(r_shadows->integer <= 3 || light->l.noShadows)
					continue;
				else
					iaType = IA_SHADOWONLY;
			}
			else
			{
				iaType = iaCache->type;
			}

			R_AddLightInteraction(light, surface->data, surface->shader, iaCache->cubeSideBits, iaType);
		}
	}
}
