/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.3
 * 
 * Copyright (c) 2013-2015, Esoteric Software
 * All rights reserved.
 * 
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to use, install, execute and perform the Spine
 * Runtimes Software (the "Software") and derivative works solely for personal
 * or internal use. Without the written permission of Esoteric Software (see
 * Section 2 of the Spine Software License Agreement), you may not (a) modify,
 * translate, adapt or otherwise create derivative works, improvements of the
 * Software or develop new applications using the Software or (b) remove,
 * delete, alter or obscure any trademarks or any copyright, trademark, patent
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 * 
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/SkeletonRenderer.h>
#include <spine/spine-cocos2dx.h>
#include <spine/extension.h>
#include <spine/PolygonBatch.h>
#include <algorithm>

USING_NS_CC;
using std::min;
using std::max;

namespace spine {

static const int quadTriangles[6] = {0, 1, 2, 2, 3, 0};

SkeletonRenderer* SkeletonRenderer::createWithData (spSkeletonData* skeletonData, bool ownsSkeletonData) {
	SkeletonRenderer* node = new SkeletonRenderer(skeletonData, ownsSkeletonData);
	node->autorelease();
	return node;
}

SkeletonRenderer* SkeletonRenderer::createWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale) {
	SkeletonRenderer* node = new SkeletonRenderer(skeletonDataFile, atlas, scale);
	node->autorelease();
	return node;
}

SkeletonRenderer* SkeletonRenderer::createWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale) {
	SkeletonRenderer* node = new SkeletonRenderer(skeletonDataFile, atlasFile, scale);
	node->autorelease();
	return node;
}

void SkeletonRenderer::initialize () {
	_worldVertices = MALLOC(float, 1000); // Max number of vertices per mesh.

	_batch = PolygonBatch::createWithCapacity(2000); // Max number of vertices and triangles per batch.
	_batch->retain();

	_blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
	setOpacityModifyRGB(true);

	setGLProgram(ShaderCache::getInstance()->getGLProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR));
}

void SkeletonRenderer::setSkeletonData (spSkeletonData *skeletonData, bool ownsSkeletonData) {
	_skeleton = spSkeleton_create(skeletonData);
	_ownsSkeletonData = ownsSkeletonData;
}

SkeletonRenderer::SkeletonRenderer ()
	: _atlas(0), _debugSlots(false), _debugBones(false), _timeScale(1) {
}

SkeletonRenderer::SkeletonRenderer (spSkeletonData *skeletonData, bool ownsSkeletonData)
	: _atlas(0), _debugSlots(false), _debugBones(false), _timeScale(1) {
	initWithData(skeletonData, ownsSkeletonData);
}

SkeletonRenderer::SkeletonRenderer (const std::string& skeletonDataFile, spAtlas* atlas, float scale)
	: _atlas(0), _debugSlots(false), _debugBones(false), _timeScale(1) {
	initWithFile(skeletonDataFile, atlas, scale);
}

SkeletonRenderer::SkeletonRenderer (const std::string& skeletonDataFile, const std::string& atlasFile, float scale)
	: _atlas(0), _debugSlots(false), _debugBones(false), _timeScale(1) {
	initWithFile(skeletonDataFile, atlasFile, scale);
}

SkeletonRenderer::~SkeletonRenderer () {
	if (_ownsSkeletonData) spSkeletonData_dispose(_skeleton->data);
	if (_atlas) spAtlas_dispose(_atlas);
	spSkeleton_dispose(_skeleton);
	_batch->release();
	FREE(_worldVertices);
    
    /*bugfixed_function_spine*/
    removeAllBoneSprite();
}

void SkeletonRenderer::initWithData (spSkeletonData* skeletonData, bool ownsSkeletonData) {
	setSkeletonData(skeletonData, ownsSkeletonData);

	initialize();
}

void SkeletonRenderer::initWithFile (const std::string& skeletonDataFile, spAtlas* atlas, float scale) {
	spSkeletonJson* json = spSkeletonJson_create(atlas);
	json->scale = scale;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonDataFile.c_str());
	CCASSERT(skeletonData, json->error ? json->error : "Error reading skeleton data.");
	spSkeletonJson_dispose(json);

	setSkeletonData(skeletonData, true);

	initialize();
}

void SkeletonRenderer::initWithFile (const std::string& skeletonDataFile, const std::string& atlasFile, float scale) {
	_atlas = spAtlas_createFromFile(atlasFile.c_str(), 0);
	CCASSERT(_atlas, "Error reading atlas file.");

	spSkeletonJson* json = spSkeletonJson_create(_atlas);
	json->scale = scale;
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonDataFile.c_str());
	CCASSERT(skeletonData, json->error ? json->error : "Error reading skeleton data file.");
	spSkeletonJson_dispose(json);

	setSkeletonData(skeletonData, true);

	initialize();
}


void SkeletonRenderer::update (float deltaTime) {
	spSkeleton_update(_skeleton, deltaTime * _timeScale);
}

void SkeletonRenderer::draw (Renderer* renderer, const Mat4& transform, uint32_t transformFlags) {
	_drawCommand.init(_globalZOrder);
	_drawCommand.func = CC_CALLBACK_0(SkeletonRenderer::drawSkeleton, this, transform, transformFlags);
	renderer->addCommand(&_drawCommand);
}

void SkeletonRenderer::drawSkeleton (const Mat4 &transform, uint32_t transformFlags) {
	getGLProgramState()->apply(transform);

	Color3B nodeColor = getColor();
	_skeleton->r = nodeColor.r / (float)255;
	_skeleton->g = nodeColor.g / (float)255;
	_skeleton->b = nodeColor.b / (float)255;
	_skeleton->a = getDisplayedOpacity() / (float)255;

	int blendMode = -1;
	Color4B color;
	const float* uvs = nullptr;
	int verticesCount = 0;
	const int* triangles = nullptr;
	int trianglesCount = 0;
	float r = 0, g = 0, b = 0, a = 0;
	for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
		spSlot* slot = _skeleton->drawOrder[i];
		if (!slot->attachment) continue;
		Texture2D *texture = nullptr;
		switch (slot->attachment->type) {
		case SP_ATTACHMENT_REGION: {
			spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
			spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
            /*bugfixed_function_spine_add===========================*/
            this->setSlotScreenPos(slot);
            if (slot->flipY)
                flipWorldVertices_Y();
            
			texture = getTexture(attachment);
			uvs = attachment->uvs;
			verticesCount = 8;
			triangles = quadTriangles;
			trianglesCount = 6;
			r = attachment->r;
			g = attachment->g;
			b = attachment->b;
			a = attachment->a;
			break;
		}
		case SP_ATTACHMENT_MESH: {
			spMeshAttachment* attachment = (spMeshAttachment*)slot->attachment;
			spMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
			texture = getTexture(attachment);
			uvs = attachment->uvs;
			verticesCount = attachment->verticesCount;
			triangles = attachment->triangles;
			trianglesCount = attachment->trianglesCount;
			r = attachment->r;
			g = attachment->g;
			b = attachment->b;
			a = attachment->a;
			break;
		}
		case SP_ATTACHMENT_SKINNED_MESH: {
			spSkinnedMeshAttachment* attachment = (spSkinnedMeshAttachment*)slot->attachment;
			spSkinnedMeshAttachment_computeWorldVertices(attachment, slot, _worldVertices);
			texture = getTexture(attachment);
			uvs = attachment->uvs;
			verticesCount = attachment->uvsCount;
			triangles = attachment->triangles;
			trianglesCount = attachment->trianglesCount;
			r = attachment->r;
			g = attachment->g;
			b = attachment->b;
			a = attachment->a;
			break;
		}
		default: ;
		} 
		if (texture) {
			if (slot->data->blendMode != blendMode) {
				_batch->flush();
				blendMode = slot->data->blendMode;
				switch (slot->data->blendMode) {
				case SP_BLEND_MODE_ADDITIVE:
					GL::blendFunc(_premultipliedAlpha ? GL_ONE : GL_SRC_ALPHA, GL_ONE);
					break;
				case SP_BLEND_MODE_MULTIPLY:
					GL::blendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case SP_BLEND_MODE_SCREEN:
					GL::blendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
					break;
				default:
					GL::blendFunc(_blendFunc.src, _blendFunc.dst);
				}
			}
			color.a = _skeleton->a * slot->a * a * 255;
			float multiplier = _premultipliedAlpha ? color.a : 255;
			color.r = _skeleton->r * slot->r * r * multiplier;
			color.g = _skeleton->g * slot->g * g * multiplier;
			color.b = _skeleton->b * slot->b * b * multiplier;
			_batch->add(texture, _worldVertices, uvs, verticesCount, triangles, trianglesCount, &color);
		}
        /*bugfixed_function_spine_add*/
        drawBoneSprite(slot);
	}
	_batch->flush();

	if (_debugSlots || _debugBones) {
		Director* director = Director::getInstance();
		director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
		director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);

		if (_debugSlots) {
			// Slots.
			DrawPrimitives::setDrawColor4B(0, 0, 255, 255);
			glLineWidth(1);
			Vec2 points[4];
			V3F_C4B_T2F_Quad quad;
			for (int i = 0, n = _skeleton->slotsCount; i < n; i++) {
				spSlot* slot = _skeleton->drawOrder[i];
				if (!slot->attachment || slot->attachment->type != SP_ATTACHMENT_REGION) continue;
				spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
				spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
				points[0] = Vec2(_worldVertices[0], _worldVertices[1]);
				points[1] = Vec2(_worldVertices[2], _worldVertices[3]);
				points[2] = Vec2(_worldVertices[4], _worldVertices[5]);
				points[3] = Vec2(_worldVertices[6], _worldVertices[7]);
				DrawPrimitives::drawPoly(points, 4, true);
			}
		}
		if (_debugBones) {
			// Bone lengths.
			glLineWidth(2);
			DrawPrimitives::setDrawColor4B(255, 0, 0, 255);
			for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
				spBone *bone = _skeleton->bones[i];
				float x = bone->data->length * bone->m00 + bone->worldX;
				float y = bone->data->length * bone->m10 + bone->worldY;
				DrawPrimitives::drawLine(Vec2(bone->worldX, bone->worldY), Vec2(x, y));
			}
			// Bone origins.
			DrawPrimitives::setPointSize(4);
			DrawPrimitives::setDrawColor4B(0, 0, 255, 255); // Root bone is blue.
			for (int i = 0, n = _skeleton->bonesCount; i < n; i++) {
				spBone *bone = _skeleton->bones[i];
				DrawPrimitives::drawPoint(Vec2(bone->worldX, bone->worldY));
				if (i == 0) DrawPrimitives::setDrawColor4B(0, 255, 0, 255);
			}
		}
		director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
	}
}

Texture2D* SkeletonRenderer::getTexture (spRegionAttachment* attachment) const {
	return (Texture2D*)((spAtlasRegion*)attachment->rendererObject)->page->rendererObject;
}

Texture2D* SkeletonRenderer::getTexture (spMeshAttachment* attachment) const {
	return (Texture2D*)((spAtlasRegion*)attachment->rendererObject)->page->rendererObject;
}

Texture2D* SkeletonRenderer::getTexture (spSkinnedMeshAttachment* attachment) const {
	return (Texture2D*)((spAtlasRegion*)attachment->rendererObject)->page->rendererObject;
}

Rect SkeletonRenderer::getBoundingBox () const {
	float minX = FLT_MAX, minY = FLT_MAX, maxX = FLT_MIN, maxY = FLT_MIN;
	float scaleX = getScaleX(), scaleY = getScaleY();
	for (int i = 0; i < _skeleton->slotsCount; ++i) {
		spSlot* slot = _skeleton->slots[i];
		if (!slot->attachment) continue;
		int verticesCount;
		if (slot->attachment->type == SP_ATTACHMENT_REGION) {
			spRegionAttachment* attachment = (spRegionAttachment*)slot->attachment;
			spRegionAttachment_computeWorldVertices(attachment, slot->bone, _worldVertices);
			verticesCount = 8;
		} else if (slot->attachment->type == SP_ATTACHMENT_MESH) {
			spMeshAttachment* mesh = (spMeshAttachment*)slot->attachment;
			spMeshAttachment_computeWorldVertices(mesh, slot, _worldVertices);
			verticesCount = mesh->verticesCount;
		} else if (slot->attachment->type == SP_ATTACHMENT_SKINNED_MESH) {
			spSkinnedMeshAttachment* mesh = (spSkinnedMeshAttachment*)slot->attachment;
			spSkinnedMeshAttachment_computeWorldVertices(mesh, slot, _worldVertices);
			verticesCount = mesh->uvsCount;
		} else
			continue;
		for (int ii = 0; ii < verticesCount; ii += 2) {
			float x = _worldVertices[ii] * scaleX, y = _worldVertices[ii + 1] * scaleY;
			minX = min(minX, x);
			minY = min(minY, y);
			maxX = max(maxX, x);
			maxY = max(maxY, y);
		}
	}
	Vec2 position = getPosition();
	return Rect(position.x + minX, position.y + minY, maxX - minX, maxY - minY);
}

// --- Convenience methods for Skeleton_* functions.

void SkeletonRenderer::updateWorldTransform () {
	spSkeleton_updateWorldTransform(_skeleton);
}

void SkeletonRenderer::setToSetupPose () {
	spSkeleton_setToSetupPose(_skeleton);
}
void SkeletonRenderer::setBonesToSetupPose () {
	spSkeleton_setBonesToSetupPose(_skeleton);
}
void SkeletonRenderer::setSlotsToSetupPose () {
	spSkeleton_setSlotsToSetupPose(_skeleton);
}

spBone* SkeletonRenderer::findBone (const std::string& boneName) const {
	return spSkeleton_findBone(_skeleton, boneName.c_str());
}

spSlot* SkeletonRenderer::findSlot (const std::string& slotName) const {
	return spSkeleton_findSlot(_skeleton, slotName.c_str());
}

bool SkeletonRenderer::setSkin (const std::string& skinName) {
	return spSkeleton_setSkinByName(_skeleton, skinName.empty() ? 0 : skinName.c_str()) ? true : false;
}
bool SkeletonRenderer::setSkin (const char* skinName) {
	return spSkeleton_setSkinByName(_skeleton, skinName) ? true : false;
}

spAttachment* SkeletonRenderer::getAttachment (const std::string& slotName, const std::string& attachmentName) const {
	return spSkeleton_getAttachmentForSlotName(_skeleton, slotName.c_str(), attachmentName.c_str());
}
bool SkeletonRenderer::setAttachment (const std::string& slotName, const std::string& attachmentName) {
	return spSkeleton_setAttachment(_skeleton, slotName.c_str(), attachmentName.empty() ? 0 : attachmentName.c_str()) ? true : false;
}
bool SkeletonRenderer::setAttachment (const std::string& slotName, const char* attachmentName) {
	return spSkeleton_setAttachment(_skeleton, slotName.c_str(), attachmentName) ? true : false;
}

spSkeleton* SkeletonRenderer::getSkeleton () {
	return _skeleton;
}

void SkeletonRenderer::setTimeScale (float scale) {
	_timeScale = scale;
}
float SkeletonRenderer::getTimeScale () const {
	return _timeScale;
}

void SkeletonRenderer::setDebugSlotsEnabled (bool enabled) {
	_debugSlots = enabled;
}
bool SkeletonRenderer::getDebugSlotsEnabled () const {
	return _debugSlots;
}

void SkeletonRenderer::setDebugBonesEnabled (bool enabled) {
	_debugBones = enabled;
}
bool SkeletonRenderer::getDebugBonesEnabled () const {
	return _debugBones;
}

void SkeletonRenderer::onEnter () {
#if CC_ENABLE_SCRIPT_BINDING
	if (_scriptType == kScriptTypeJavascript && ScriptEngineManager::sendNodeEventToJSExtended(this, kNodeOnEnter)) return;
#endif
	Node::onEnter();
	scheduleUpdate();
}

void SkeletonRenderer::onExit () {
#if CC_ENABLE_SCRIPT_BINDING
	if (_scriptType == kScriptTypeJavascript && ScriptEngineManager::sendNodeEventToJSExtended(this, kNodeOnExit)) return;
#endif
	Node::onExit();
	unscheduleUpdate();
}

// --- CCBlendProtocol

const BlendFunc& SkeletonRenderer::getBlendFunc () const {
	return _blendFunc;
}

void SkeletonRenderer::setBlendFunc (const BlendFunc &blendFunc) {
	_blendFunc = blendFunc;
}

void SkeletonRenderer::setOpacityModifyRGB (bool value) {
	_premultipliedAlpha = value;
}

bool SkeletonRenderer::isOpacityModifyRGB () const {
	return _premultipliedAlpha;
}

    
/*bugfixed_function_spine========================================begin*/
cocos2d::Sprite* SkeletonRenderer::getSpriteForBone(const std::string& boneName)
{
    BoneSpriteIter iter = _boneSprites.find(boneName);
    if (iter != _boneSprites.end())
    {
        BoneSprite& boneNode = iter->second;
        return boneNode.sprite;
    }
    return nullptr;
}

void SkeletonRenderer::removeBoneSprite(const std::string& boneName)
{
    BoneSpriteIter iter = _boneSprites.find(boneName);
    if (iter != _boneSprites.end())
    {
        _boneSprites.erase(boneName);
        BoneSprite& boneSprite = iter->second;
        boneSprite.sprite->onExit();
        CC_SAFE_RELEASE_NULL(boneSprite.sprite);
    }
    else
    {
        log("not find Sprite on Bone named : %s",boneName.c_str());
    }
}

void SkeletonRenderer::removeAllBoneSprite()
{
    for (auto it = _boneSprites.begin() ; it != _boneSprites.end(); ++it)
    {
        BoneSprite& boneSprite = it->second;
        boneSprite.sprite->onExit();
        CC_SAFE_RELEASE_NULL(boneSprite.sprite);
    }
    _boneSprites.clear();
}

void SkeletonRenderer::addSpriteToBone(const std::string& boneName,cocos2d::Sprite* sprite)
{
    spBone* bone = findBone(boneName);
    if (bone)
    {
        this->removeBoneSprite(boneName);
        BoneSprite boneSprite;
        boneSprite.bone = bone;
        boneSprite.sprite = sprite;
        boneSprite.sprite->onEnter();
        boneSprite.isToSlot = 0;
        boneSprite.flipY = 0;
        setBoneSprite(&boneSprite);
        CC_SAFE_RETAIN(boneSprite.sprite);
        
        _boneSprites.insert(BoneSpriteMap::value_type(boneName,boneSprite));
    }else
    {
        log("not find bone named : %s",boneName.c_str());
        return;
    }
}

void SkeletonRenderer::addSpriteToSlot(const std::string& slotName,cocos2d::Sprite* sprite,bool flipY)
{
    spSlot* slot = findSlot(slotName);
    if (slot)
    {
        if(slot->attachment->type != spAttachmentType::SP_ATTACHMENT_REGION)
        {
            log("only can add Sprite to Slot with SP_ATTACHMENT_REGION");
            return;
        }
        
        this->removeBoneSprite(slotName);
        BoneSprite boneSprite;
        boneSprite.bone = slot->bone;
        boneSprite.sprite = sprite;
        boneSprite.sprite->onEnter();
        boneSprite.isToSlot = 1;
        boneSprite.flipY = flipY;
        setBoneSprite(&boneSprite);
        CC_SAFE_RETAIN(boneSprite.sprite);
        
        _boneSprites.insert(BoneSpriteMap::value_type(slotName,boneSprite));
    }else
    {
        log("not find slot named : %s",slotName.c_str());
        return;
    }
}
    
void SkeletonRenderer::setBoneSprite(BoneSprite* boneSprite)
{
    boneSprite->orginRotation = acosf(boneSprite->bone->m00);
    
    Sprite* sprite = boneSprite->sprite;
    Size size = sprite->getContentSize();
    float scaleX= sprite->getScaleX();
    float scaleY= sprite->getScaleY();
    float rotation = sprite->getRotation();
    float x = sprite->getPositionX();
    float y = sprite->getPositionY();
    
    boneSprite->uvs[SP_VERTEX_X1] = 0;
    boneSprite->uvs[SP_VERTEX_Y1] = 1;
    boneSprite->uvs[SP_VERTEX_X2] = 0;
    boneSprite->uvs[SP_VERTEX_Y2] = 0;
    boneSprite->uvs[SP_VERTEX_X3] = 1;
    boneSprite->uvs[SP_VERTEX_Y3] = 0;
    boneSprite->uvs[SP_VERTEX_X4] = 1;
    boneSprite->uvs[SP_VERTEX_Y4] = 1;
    
    float localX = -size.width / 2 * scaleX ;
    float localY = -size.height / 2 * scaleY ;
    float localX2 = localX + size.width * scaleX;
    float localY2 = localY + size.height * scaleY;
    float radians = -rotation * DEG_RAD;
    float cosine = COS(radians), sine = SIN(radians);
    float localXCos = localX * cosine + x;
    float localXSin = localX * sine;
    float localYCos = localY * cosine + y;
    float localYSin = localY * sine;
    float localX2Cos = localX2 * cosine + x;
    float localX2Sin = localX2 * sine;
    float localY2Cos = localY2 * cosine + y;
    float localY2Sin = localY2 * sine;
    boneSprite->offset[SP_VERTEX_X1] = localXCos - localYSin;
    boneSprite->offset[SP_VERTEX_Y1] = localYCos + localXSin;
    boneSprite->offset[SP_VERTEX_X2] = localXCos - localY2Sin;
    boneSprite->offset[SP_VERTEX_Y2] = localY2Cos + localXSin;
    boneSprite->offset[SP_VERTEX_X3] = localX2Cos - localY2Sin;
    boneSprite->offset[SP_VERTEX_Y3] = localY2Cos + localX2Sin;
    boneSprite->offset[SP_VERTEX_X4] = localX2Cos - localYSin;
    boneSprite->offset[SP_VERTEX_Y4] = localYCos + localX2Sin;
}

void SkeletonRenderer::computeWorldVertices(BoneSprite* boneSprite,float* vertices)
{
    spBone* bone = boneSprite->bone;
    float* offset = boneSprite->offset;
    float cosineOffset = COS(boneSprite->orginRotation), sineOffset = SIN(boneSprite->orginRotation);
    float cosine = bone->m00 * cosineOffset + bone->m10 * sineOffset;
    float sine = bone->m10 * cosineOffset - sineOffset * bone->m00;
    
    float x = bone->skeleton->x + bone->worldX ;
    float y = bone->skeleton->y + bone->worldY ;
    
    vertices[SP_VERTEX_X1] = offset[SP_VERTEX_X1] * cosine + offset[SP_VERTEX_Y1] * (-sine) + x;
    vertices[SP_VERTEX_Y1] = offset[SP_VERTEX_X1] * sine + offset[SP_VERTEX_Y1] * cosine + y;
    vertices[SP_VERTEX_X2] = offset[SP_VERTEX_X2] * cosine + offset[SP_VERTEX_Y2] * (-sine) + x;
    vertices[SP_VERTEX_Y2] = offset[SP_VERTEX_X2] * sine + offset[SP_VERTEX_Y2] * cosine + y;
    vertices[SP_VERTEX_X3] = offset[SP_VERTEX_X3] * cosine + offset[SP_VERTEX_Y3] * (-sine) + x;
    vertices[SP_VERTEX_Y3] = offset[SP_VERTEX_X3] * sine + offset[SP_VERTEX_Y3] * cosine + y;
    vertices[SP_VERTEX_X4] = offset[SP_VERTEX_X4] * cosine + offset[SP_VERTEX_Y4] * (-sine) + x;
    vertices[SP_VERTEX_Y4] = offset[SP_VERTEX_X4] * sine + offset[SP_VERTEX_Y4] * cosine + y;
    
    Vec2 skeletonPos = this->getPosition();
    //设置精灵坐标，精灵调用convertToWorldSpace时才能获取正确的屏幕坐标
    boneSprite->sprite->setPosition(skeletonPos + Vec2(bone->worldX,bone->worldY));
}

void SkeletonRenderer::drawBoneSprite(spSlot* slot)
{
    if (slot->attachment->type == spAttachmentType::SP_ATTACHMENT_REGION)
    {
        auto iter = _boneSprites.find(slot->bone->data->name);
        if (iter != _boneSprites.end())
        {
            BoneSprite& boneSprite = iter->second;
            Sprite* sprite = boneSprite.sprite;
            
            
            _batch->flush();
            GL::blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            
            Color4B color;
            color.a = sprite->getOpacity();
            color.r = sprite->getColor().r;
            color.g = sprite->getColor().g;
            color.b = sprite->getColor().b;
            
            if (boneSprite.isToSlot)
            {
                if (boneSprite.flipY)
                    flipWorldVertices_Y();
                _batch->add(sprite->getTexture(), _worldVertices, boneSprite.uvs, 8, quadTriangles, 6, &color);
                _batch->flush();
                GL::blendFunc(_blendFunc.src, _blendFunc.dst);
                return;
            }
            
            computeWorldVertices(&boneSprite, _worldVertices);
            
            //添加至渲染
            _batch->add(sprite->getTexture(), _worldVertices, boneSprite.uvs, 8, quadTriangles, 6, &color);
            _batch->flush();
            GL::blendFunc(_blendFunc.src, _blendFunc.dst);
        }
    }else if (slot->attachment->type == spAttachmentType::SP_ATTACHMENT_SKINNED_MESH)
    {
        spSkinnedMeshAttachment* skinedMeshAttachment = (spSkinnedMeshAttachment*)slot->attachment;
        spBone** skeletonBones = slot->bone->skeleton->bones;
        
        int v = 0;
        for (; v < skinedMeshAttachment->bonesCount; )
        {
            const int nn = skinedMeshAttachment->bones[v] + v;
            v++;
            for (; v <= nn; v++)
            {
                const spBone* bone = skeletonBones[skinedMeshAttachment->bones[v]];
                auto iter = _boneSprites.find(bone->data->name);
                if (iter != _boneSprites.end())
                {
                    BoneSprite& boneSprite = iter->second;
                    Color4B color;
                    
                    _batch->flush();
                    GL::blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    
                    Sprite* sprite = boneSprite.sprite;
                    
                    color.a = sprite->getOpacity();
                    color.r = sprite->getColor().r;
                    color.g = sprite->getColor().g;
                    color.b = sprite->getColor().b;
                    
                    computeWorldVertices(&boneSprite, _worldVertices);
                    
                    //添加至渲染
                    _batch->add(sprite->getTexture(), _worldVertices, boneSprite.uvs, 8, quadTriangles, 6, &color);
                    _batch->flush();
                    GL::blendFunc(_blendFunc.src, _blendFunc.dst);
                    
                    return;
                }
            }
        }
    }
}
    
/*纯粹为了方便，定义一个交换函数*/
void swapVertices(float &x,float &y)
{
    float temp = x;
    x =  y;
    y = temp;
}

void SkeletonRenderer::setSlotScreenPos(spSlot* slot)
{
    //左下角
    slot->lbX = _worldVertices[SP_VERTEX_X1] + this->getPositionX();
    slot->lbY = _worldVertices[SP_VERTEX_Y1] + this->getPositionY();
    //左上角
    slot->ltX = _worldVertices[SP_VERTEX_X2] + this->getPositionX();
    slot->ltY = _worldVertices[SP_VERTEX_Y2] + this->getPositionY();
    //右上角
    slot->rtX = _worldVertices[SP_VERTEX_X3] + this->getPositionX();
    slot->rtY = _worldVertices[SP_VERTEX_Y3] + this->getPositionY();
    //右下角
    slot->rbX = _worldVertices[SP_VERTEX_X4] + this->getPositionX();
    slot->rbY = _worldVertices[SP_VERTEX_Y4] + this->getPositionY();

}
    
void SkeletonRenderer::flipWorldVertices_Y()
{
    swapVertices(_worldVertices[SP_VERTEX_X1], _worldVertices[SP_VERTEX_X2]);
    swapVertices(_worldVertices[SP_VERTEX_Y1], _worldVertices[SP_VERTEX_Y2]);
    swapVertices(_worldVertices[SP_VERTEX_X3], _worldVertices[SP_VERTEX_X4]);
    swapVertices(_worldVertices[SP_VERTEX_Y3], _worldVertices[SP_VERTEX_Y4]);
}

void SkeletonRenderer::setSlotFlipY(const std::string& slotName,bool flip)
{
    spSlot* slot = findSlot(slotName);
    if (slot)
    {
        slot->flipY = flip;
    }
    else
    {
        log("not find slot named : %s",slotName.c_str());
    }
}
/*============================================================end*/
}
