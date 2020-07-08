//
//  JdModuleId.hpp
//
//  Created by Steven Massey on 6/10/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#ifndef JdModuleId_hpp
#define JdModuleId_hpp

#include "JdRandom.hpp"
#include "JdCoreL2.hpp"

d_jdStruct (JdId)
{
	static const u64 c_instanceIdMask		= std::numeric_limits <u64>::max () >> 4;	// top bit unused + 3 flag bits

	static const u64 c_namedInstanceFlag	= 1LL << 60;

//	JdModuleId (const JdModuleId & i_id)
//	{
//		m_id = i_id.m_id;
//	}
	
	JdId () { }
	JdId (u64 i_id) : m_id (i_id) { }

	JdId (cstr_t i_moduleName)
	{
		m_id = c_namedInstanceFlag | Jd::HashString64 (i_moduleName);
	}
	
	JdId (stringRef_t i_moduleName)
	{
		m_id = c_namedInstanceFlag | Jd::HashString64 (i_moduleName);
	}

//	bool		IsInstanceId	() const
//	{
//		return ((m_id & c_moduleIdFlag) == 0);
//	}

//	bool		IsModuleId		() const
//	{
//		return (m_id & c_moduleIdFlag);
//	}
	
//	void		SetModuleId		(u64 i_id)
//	{
//		m_id = i_id | c_moduleIdFlag;
//	}
	
	operator u64				() const
	{
		return m_id;// & ~c_moduleIdFlag;
	}
	
	JdId & operator =		(const JdId & i_moduleId)
	{
		m_id = i_moduleId.m_id;
		return * this;
	}

	static
	JdId		New			()
	{
		return JdId (JdRandom::Get <u64> () & c_instanceIdMask);
	}

	protected:
	u64				m_id		= 0;
};



inline std::ostream & operator << (std::ostream & output, JdIdRef i_moduleId)
{
	char temp [64];
//	if (i_moduleId.IsInstanceId ()) output << "i.";
	sprintf (temp, "i.%016llX", (u64) i_moduleId);
	output << temp;
	return output;
}

const JdId c_jdNullModuleId;

#endif /* JdModuleId_hpp */
