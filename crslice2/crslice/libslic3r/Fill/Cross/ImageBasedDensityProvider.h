//Copyright (c) 2017 Tim Kuipers
//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef ORCA_INFILL_IMAGE_BASED_DENSITY_PROVIDER_H
#define ORCA_INFILL_IMAGE_BASED_DENSITY_PROVIDER_H
#include "Point.hpp"
#include "libslic3r/BoundingBox.hpp"
//#include "utils/AABB.h"
//
//#include "DensityProvider.h"

namespace Slic3r::Cross
{

//struct AABB3D;

	/*!
 * Parent class of function objects which return the density required for a given region.
 *
 * This density requirement can be based on user input, distance to the 3d model shell, Z distance to top skin, etc.
 */
	class DensityProvider
	{
	public:
		/*!
		 * \return the approximate required density of a cube
		 */
		virtual float operator()(const BoundingBox3& aabb) const = 0;
		virtual ~DensityProvider()
		{
		};
	};

	class UniformDensityProvider : public DensityProvider
	{
	public:
		UniformDensityProvider(float density)
			: density(density)
		{
		};

		virtual ~UniformDensityProvider()
		{
		};

		virtual float operator()(const BoundingBox3&) const
		{
			return density;
		};
	protected:
		float density;
	};


class ImageBasedDensityProvider : public DensityProvider
{
public:
    ImageBasedDensityProvider(const std::string filename, const BoundingBox aabb);

    virtual ~ImageBasedDensityProvider();

    virtual float operator()(const BoundingBox3& aabb) const;

protected:
    Vec3crd image_size; //!< dimensions of the image. Third dimension is the amount of channels.
    unsigned char* image = nullptr; //!< image data: rows of channel data per pixel.

    BoundingBox print_aabb; //!< bounding box of print coordinates in which to apply the image
};

} // namespace cura52


#endif // ORCA_INFILL_IMAGE_BASED_DENSITY_PROVIDER_H
