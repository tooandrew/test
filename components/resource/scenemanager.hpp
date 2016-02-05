#ifndef OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SCENEMANAGER_H

#include <string>
#include <map>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Texture>

namespace Resource
{
    class ImageManager;
    class NifFileManager;
}

namespace VFS
{
    class Manager;
}

namespace osgUtil
{
    class IncrementalCompileOperation;
}

namespace osgDB
{
    class ObjectCache;
}

namespace osgViewer
{
    class Viewer;
}

namespace Resource
{

    /// @brief Handles loading and caching of scenes, e.g. .nif files or .osg files
    /// @note Some methods of the scene manager can be used from any thread, see the methods documentation for more details.
    class SceneManager
    {
    public:
        SceneManager(const VFS::Manager* vfs, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager);
        ~SceneManager();

        /// Get a read-only copy of this scene "template"
        /// @note If the given filename does not exist or fails to load, an error marker mesh will be used instead.
        ///  If even the error marker mesh can not be found, an exception is thrown.
        /// @note Thread safe.
        osg::ref_ptr<const osg::Node> getTemplate(const std::string& name);

        /// Create an instance of the given scene template
        /// @see getTemplate
        /// @note Thread safe.
        osg::ref_ptr<osg::Node> createInstance(const std::string& name);

        /// Create an instance of the given scene template and immediately attach it to a parent node
        /// @see getTemplate
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        osg::ref_ptr<osg::Node> createInstance(const std::string& name, osg::Group* parentNode);

        /// Attach the given scene instance to the given parent node
        /// @note You should have the parentNode in its intended position before calling this method,
        ///       so that world space particles of the \a instance get transformed correctly.
        /// @note Assumes the given instance was not attached to any parents before.
        /// @note Not thread safe, unless parentNode is not part of the main scene graph yet.
        void attachTo(osg::Node* instance, osg::Group* parentNode) const;

        /// Manually release created OpenGL objects for the given graphics context. This may be required
        /// in cases where multiple contexts are used over the lifetime of the application.
        void releaseGLObjects(osg::State* state);

        /// Set up an IncrementalCompileOperation for background compiling of loaded scenes.
        void setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation* ico);

        /// @note SceneManager::attachTo calls this method automatically, only needs to be called by users if manually attaching
        void notifyAttached(osg::Node* node) const;

        const VFS::Manager* getVFS() const;

        Resource::ImageManager* getImageManager();

        /// @param mask The node mask to apply to loaded particle system nodes.
        void setParticleSystemMask(unsigned int mask);

        /// @param viewer used to apply the new filter settings to the existing scene graph. If there is no scene yet, you can pass a NULL viewer.
        void setFilterSettings(const std::string &magfilter, const std::string &minfilter,
                               const std::string &mipmap, int maxAnisotropy,
                               osgViewer::Viewer *viewer);

        /// Apply filter settings to the given texture. Note, when loading an object through this scene manager (i.e. calling getTemplate or createInstance)
        /// the filter settings are applied automatically. This method is provided for textures that were created outside of the SceneManager.
        void applyFilterSettings (osg::Texture* tex);

        /// Keep a copy of the texture data around in system memory? This is needed when using multiple graphics contexts,
        /// otherwise should be disabled to reduce memory usage.
        void setUnRefImageDataAfterApply(bool unref);

    private:
        const VFS::Manager* mVFS;
        Resource::ImageManager* mImageManager;
        Resource::NifFileManager* mNifFileManager;

        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        int mMaxAnisotropy;
        bool mUnRefImageDataAfterApply;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        unsigned int mParticleSystemMask;

        osg::ref_ptr<osgDB::ObjectCache> mCache;

        SceneManager(const SceneManager&);
        void operator = (const SceneManager&);
    };

}

#endif
