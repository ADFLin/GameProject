#ifndef serialization_h__
#define serialization_h__

#include "common.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_free.hpp>


#include <exception>
#include <fstream>
#include <vector>


BOOST_SERIALIZATION_SPLIT_FREE(unsigned)

namespace boost{
namespace serialization{

	typedef boost::mpl::bool_<true>  true_type;
	typedef boost::mpl::bool_<false> false_type;


	template < class Archive >
	void serialize( Archive & ar, Vec3D& v , const unsigned int /* file_version */ )
	{
		ar & v[0] & v[1] & v[2];
	}

	template < class Archive >
	void serialize( Archive & ar, Quat& q , const unsigned int /* file_version */ )
	{
		ar & q[0] & q[1] & q[2] & q[3];
	}


} // namespace serialization
} // namespace boost




#endif // serialization_h__
