/*
 *  JdDbTable.hpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/11/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
*/

#pragma once

#include <sqlite3.h>

#define d_jdEnableEnvironment 1

# include "IJdSql.h"



namespace c_jdSql
{
	const int rowReady = SQLITE_ROW;
	const int done = SQLITE_DONE;
}

#include <stdio.h>

//#include <city.h>
#include <iostream>
#include <vector>
#include "JdUUID.hpp"
#include "Epigram.hpp"
#include "EpAttribute.hpp"
#include "Epilog.hpp"

using std::vector, std::string;

// FIX: move this to JdCore/JdInternal?
//std::ostream & operator << (std::ostream &i_stream, const uint128 &i_hash);


d_jdErrorConst (Db, rowNotFound, "database row wasn't found");
d_jdErrorConst (Db, rowAlreadyExtant, "database row already created");

const char * const c_jdDatabaseTablePrefix = "jdu_";

/*
		FIX: these notes are out of date
	
 
 - Every row is identified with a UUID
 - Any named element can be added to a row. Columns are inserted as needed to satisfy new element names.
 
 To create a row:
 
 float f = 3.14;
 int x = 123;
 JdUUID uuid (true);
 
 db.CreateRow (uuid, msg_ ("column_a", f) ("column_b", x));
 
 To fetch a row:
 
 Epigram row = db.GetRow (uuid);
 float f = row ["column_a"];
 
 To find a row:
 
 JdUuidDatabase::rows_t rows = db.Select ("column_b", 123). Select ("column_a, 3.14). Query();		// Selects can be compounded (ANDed)
 
 if (rows.size())
 {
	Epigram row = db.GetRow (rows[0]);
 }
 
 TODO:
 - GetRow() after a Select/Query requeries SQL. Query() could reset the SQL iterator and GetRow could check for pre-existing match before running a fresh SELECT
 - There could be more caching in general to avoid rerunning redundant SELECTs.
 - Statements can be precompiled.
 
	
	Usage notes
	-----------
	- arrays of fundamentals are stored as Blobs (but still with the actual /t flag in the column name). An epigram doesn't distinguish between a single value
	and an array of 1, so this can be problematic; either need to have column upgrade mechnaism and/or have a DefineTable () api.
 
 */



template <typename rowid_t>
class JdSqlTableT  //: JdModuleHelper <IJdModuleServer>
{
	public: //---------------------------------------------------------------------------------------------------------------------------------------------
	
	typedef vector <rowid_t> rows_t;

	struct interface // documentation purposes only:
	{
		bool								HasRow						(const rowid_t &i_rowId);
		
		i64									CountRows					();
		
		rows_t								SelectAll					();
		rows_t								Select						(EpigramRef i_selects);
		
		// convenience function.  returns row data iff one row result
		Epigram								SelectRow					(EpigramRef i_selects);
		
		Epigram								GetRow						(const rowid_t &i_uuid);
		
		JdResult							ModifyRow					(const rowid_t &i_uuid, EpigramRef i_columns);
		JdResult							EraseRow					(const rowid_t &i_uuid);

		JdResult							CreateOrModifyRow			(const rowid_t & i_rowId, EpigramRef i_columns);
		
		// native sql row-id (i64):
		JdResult							CreateRow					(EpigramRef i_columns, rowid_t * o_rowId = nullptr);
		
		// JdUUID row-id:
		JdResult							CreateRow					(const rowid_t &		i_uuid,
																		 EpigramRef				i_columns = Epigram (),
																		 bool					i_eraseExistentRow = false);
	};

	//------------------------------------------------------------------------------------------------------------------------------------------------------

	JdSqlTableT					(cstr_t i_databaseName, cstr_t i_rowIdName, u64 * i_linkedSequence)
	:
	m_tablesInitialized			(false),
	m_rowIdName					(i_rowIdName),
	m_sequence					(& m_localSequence)
	{
		d_jdAssert (strstr (i_databaseName,"-") == 0, "sql restricts usage of -, use an underscore");
		
		if (i_linkedSequence) m_sequence = i_linkedSequence;
//		m_useNativeSqliteRowId = (string (m_rowIdName) == "rowid");
	
		if (! JdConfig::IsPublicBuild ())
			m_databaseName = c_jdDatabaseTablePrefix;		// JAE version has pointless jdu_prefix....		QUE: kill this before release???

		m_databaseName += i_databaseName;
	}

	

	
	JdResult							DropTable					()
	{
		JdResult result;
		
		m_f << "DROP TABLE IF EXISTS ", m_databaseName;
		
		jd_lock (jd_sql)
		{
			result = jd_sql->Prepare (m_f);
			if (not result)
				result = QuietSqlResult (jd_sql->Step ());
		}
		
		return result;
	}
	

	struct constraint
	{
		const u32 unique		= 1;
	};
	
	void								DefineColumns				(stringRef_t i_sql)
	{
		m_columnDef = i_sql;
		SetupDatabase ();
	}
	
	template <typename T>
	JdResult							DefineColumn				(cstr_t i_name, u32 i_constraints)
	{
		m_definedColumns.push_back ({ Jd::TypeIdChar <T>(), Jd::TypeName <T> (), i_name, i_constraints });
		
		return c_jdNoErr;
	}
	
	u64 *								GetSequencer				()
	{
		return m_sequence;
	}
    
	u64									GetSequenceNum				()
	{
		return * m_sequence;
	}
	
	u64									IncrementSequenceNum		()
	{
		u64 & sequence = * m_sequence;
		return ++sequence;
	}
	
	bool								HasRow						(const rowid_t &i_rowId)
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
		m_f << "SELECT * FROM ", m_databaseName, " WHERE ", m_rowIdName, "=?";
		
        JdResult result;
        
        jd_lock (jd_sql)
        {
            result = jd_sql->Prepare (m_f);
            
            if (result) return false;
            
			BindRowId (jd_sql, 0, i_rowId);
            
            result = jd_sql->Step ();
        }
        
		return (result == c_jdSql::rowReady);
	}
	

	i64									CountRows					()
	{
		SetupDatabase(); d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
		m_f << "SELECT COUNT(*) FROM ", m_databaseName;
		
		i64 numRows = 0;
		
		jd_lock (jd_sql)
		{
			JdResult result = jd_sql->Prepare (m_f);
			
			if (not result)
			{
				result = jd_sql->Step ();
				
				if (result == c_jdSql::rowReady)
				{
					auto columns = jd_sql->GetColumnCount ();
					
					if (columns)
						numRows = jd_sql->GetColumn64 (0);
				}
			}
		}
		
		return numRows;
	}

	
	rows_t								Select						(EpigramRef i_selectInfo)
	{
		rows_t rows;
		
		m_query << "SELECT * FROM ", m_databaseName, " WHERE ";
		
		if (not i_selectInfo.IsEmpty())
		{
			QueryGenerator <65536> generator;
			
			string sql = generator.EncodeColumns (i_selectInfo, " AND ");
			m_query += sql;
            
			JdResult result;
            
			jd_lock (jd_sql)
            {
                result = jd_sql->Prepare (m_query);
                m_query.Reset();
                
                if (not result)
                {
                    generator.BindBlobs (jd_sql);
                    rows = ProcessQuery ();
                }
            }
		}
		
		return rows;
	}
	

	rows_t							SelectAll						()
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
        m_query << "SELECT * FROM ", m_databaseName;
		
        rows_t rows;
        
        jd_lock (jd_sql)
        {
            JdResult result = jd_sql->Prepare (m_query);
            m_query.Reset();
            
            if (not result) rows = ProcessQuery();
        }
        
        return rows;
	}
	
	
	JdResult							CreateOrModifyRow			(const rowid_t & i_rowId, EpigramRef i_columns)
	{
		if (HasRow (i_rowId))	return ModifyRow (i_rowId, i_columns);
		else					return CreateRow (i_rowId, i_columns);
	}


	void InvalidateRowId (i64 * o_rowId)
	{
		if (o_rowId)
			* o_rowId = 0;
	}
	
	void InvalidateRowId (JdUUID * o_rowId)
	{
		if (o_rowId)
			* o_rowId = JdUUID ();
	}

	
	
	// This method is intended for native sql rowid's only. (that is, JdDatabase rather than JdUuidDatabase)
	JdResult							CreateRow					(EpigramRef i_columns, rowid_t * o_rowId = nullptr)
	{
		JdResult result;
		
		InvalidateRowId (o_rowId);
		
//		cout << "CREATE: "; Epigram e (i_columns); e.Dump ();
//		i_columns.dump ();

		SetupDatabase ();											d_jdAssert (m_tablesInitialized, "database wasn't initialized");

		m_f << "INSERT INTO ", m_databaseName, "(seq) VALUES (", IncrementSequenceNum (), ")";
		
		i64 rowId = 0;
        jd_lock (jd_sql)
        {
            result = jd_sql->Prepare (m_f);
			
            if (not result)
			{
				result = QuietSqlResult (jd_sql->Step ());

				if (not result)
				{
					rowId = jd_sql->GetLastRowId ();
					if (o_rowId) * o_rowId = rowId;
					
					if (i_columns.HasElements ())
						result = ModifyRowInternal (rowId, i_columns);
				}
			}
		}
		
		return result;
	}

	
	// FIX: this could be more efficient, implemented in one SQL step.
	JdResult							CreateRow					(const rowid_t &		i_uuid,
																	 EpigramRef				i_columns = Epigram (),
																	 bool					i_eraseExistentRow = false)
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
//		d_jdAssert (i_uuid.IsSet(), "uuid is null");
		
//		i_columns.Dump ();
		
		if (not i_eraseExistentRow)
		{
			if (HasRow (i_uuid))
			{
				if (i_columns)	return c_jdDb::rowAlreadyExtant;
				else			return c_jdNoErr;
			}
		}
		
		m_f << "INSERT INTO ", m_databaseName, "(", m_rowIdName, ",seq) VALUES (?,", IncrementSequenceNum (), ")";
		
        JdResult result;
        jd_lock (jd_sql)
        {
            result = jd_sql->Prepare (m_f);
		
			if (not result)
			{
				result = BindRowId (jd_sql, 0, i_uuid);
			}
			
            if (not result) result = jd_sql->Step ();
			
			if (result == c_jdSql::done)
				result = c_jdNoErr;
			
			if (i_columns.HasElements ())
				result = ModifyRowInternal (i_uuid, i_columns);
		}
	
		return result;
	}
	
	
	JdResult							EraseRow						(const rowid_t &i_uuid)
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
		m_f << "DELETE FROM ", m_databaseName, " WHERE ", m_rowIdName, "=?";
		
		JdResult result;
        jd_lock (jd_sql)
        {
            result = jd_sql->Prepare (m_f);
            if (result) return result;
            
			BindRowId (jd_sql, 0, i_uuid);
            
            result = jd_sql->Step ();
        }
        
		if (result == c_jdSql::done)
			result = c_jdNoErr;
		
		return result;
	}
	
	
	Epigram						SelectRow					(EpigramRef i_selects)
	{
		Epigram e;
		
		auto r = Select (i_selects);
		
		if (r.size () == 1)
			e = GetRow (r [0]);
		
		return e;
	}
	
	static Epigram			EncodeRow					(IJdSql & i_sql, i32 i_startColumn, bool i_typedColumns)
	{
		Epigram e;
		
		for (i32 c = i_startColumn; c < i_sql->GetColumnCount (); ++c)
		{
			cstr_t name = i_sql->GetColumnName (c);
			i32 columnType = i_sql->GetColumnType (c);
			
			cstr_t columnName;
			u32 typeLength;
			u8 dataTypeChar;
			cstr_t typeInfo;
			
			JdString128 cn;

			if (i_typedColumns)
			{
				cstr_t slash = strstr (name, "/");
				if (not slash)
					continue;
				
				cstr_t elementNameEnd = slash;
				
				typeInfo = slash + 1;
				typeLength = (u32) strlen (typeInfo);
				
				dataTypeChar = typeInfo [0];
				
				columnName = cn.Set (name, elementNameEnd);
			}
			else
			{
				columnName = name;
				typeLength = 1;
				dataTypeChar = 0;
				typeInfo = nullptr;
				
				switch (columnType)
				{
					case e_jdSql_typeInteger:
						dataTypeChar = Jd::TypeIdChar <i64> ();
						break;
					case e_jdSql_typeFloat:
						dataTypeChar = Jd::TypeIdChar <f64> ();
						break;
				}
			}

			switch (columnType)
			{
				case e_jdSql_typeText:
				{
					d_jdAssert (typeLength == 1, "sql/epmsg type id is too long");
					cstr_t text = i_sql->GetColumnText (c);
					e (columnName, text);
					break;
				}
				case e_jdSql_typeInteger:
				{
					//                                d_jdAssert (typeLength == 1, "sql/epmsg type id is too long");
					
					i64 v = i_sql->GetColumn64 (c);
					
//					jd::out ("'@' @", columnName, (voidpr_t) columnName);
					
					
					if (dataTypeChar == Jd::TypeIdChar <i32> ()) 			e (columnName, (i32) v);
					else if (dataTypeChar == Jd::TypeIdChar <u32> ()) 		e (columnName, (u32) v);
					else if (dataTypeChar == Jd::TypeIdChar <i64> ())		e (columnName, (i64) v);
					else if (dataTypeChar == Jd::TypeIdChar <u64> ()) 		e (columnName, (u64) v);
					else if (dataTypeChar == Jd::TypeIdChar <i16> ()) 		e (columnName, (i16) v);
					else if (dataTypeChar == Jd::TypeIdChar <u16> ()) 		e (columnName, (u16) v);
					else if (dataTypeChar == Jd::TypeIdChar <i8> ()) 		e (columnName, (i8) v);
					else if (dataTypeChar == Jd::TypeIdChar <u8> ()) 		e (columnName, (u8) v);
					
					else if (dataTypeChar == Jd::TypeIdToChar (c_jdTypeId::enumeration))
					{
						Jd::_7bRE enumeration (v);
						
						auto enumName = typeInfo + 2;
						e.AddRawObject (columnName, c_jdTypeId::enumeration, enumName, 0, enumeration.bytes, enumeration.numBytes);
					}
					
					break;
				}
				case e_jdSql_typeFloat:
				{
					d_jdAssert (typeLength == 1, "sql/epmsg type id is too long");
					
					f64 v = i_sql->GetColumnFloat (c);
					
					if (dataTypeChar == Jd::TypeIdChar <f32> ()) 			e (columnName, (f32) v);
					else if (dataTypeChar == Jd::TypeIdChar <f64> ()) 		e (columnName, (f64) v);
					break;
				}
				case e_jdSql_typeBlob:
				{
					const u8 * data = (const u8 *) i_sql->GetColumnBlob (c);
					u32 size = i_sql->GetColumnNumBytes (c);
					
					if (data and size)
					{
						u8 typeId = Jd::TypeCharToId (dataTypeChar);
						
						if (typeLength == 1)
						{
							if (typeId == c_jdTypeId::epigram)
							{
								e (columnName, ConstEpigram (data, size));
							}
							else if (typeId == c_jdTypeId::binary) // binary
							{
								e (columnName, EpBinary (data,size));
							}
							else if (typeId == c_jdTypeId::uuid) // binary
							{
								e (columnName, JdUUID (data));
							}
							else if (typeId == c_jdTypeId::f64)
							{
								// FIX: quick hack to get f64 array goin
								vector <f64> v (size / Jd::TypeIdToSize (typeId));
								memcpy (v.data (), data, size);
								
								e (columnName, v);
							}
						}
						else
						{
							cstr_t className = typeInfo + 2;		// e.g. "columnName/O/ClassName"
							
							if (typeId == c_jdTypeId::versionedObject)
							{
								u8 version = data [0];
								e.AddRawObject (columnName, typeId, className, version, data+1, size-1);
							}
							else
								e.AddRawObject (columnName, typeId, className, 0, data, size);
						}
					}
				}
			}
		}
		
		return e;
	}
	
	
	Epigram					GetRow						(const rowid_t & i_rowId)
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");

		Epigram e;
		
		m_f << "SELECT * FROM ", m_databaseName, " WHERE ", m_rowIdName, "=?";
		
		jd_lock (jd_sql)
        {
            JdResult result = jd_sql->Prepare (m_f);
            
			BindRowId (jd_sql, 0, i_rowId);
            
            if (not result)
            {
                result = jd_sql->Step ();
				
                if (result == c_jdSql::rowReady)
                {
					e = EncodeRow (jd_sql, m_firstUserColumn, true);
                }
            }
        }
		
		return e;
	}
    
	
	template <const i32 t_blobStorageSize>
	struct QueryGenerator
	{
		string							EncodeColumns				(EpigramRef info, cstr_t i_bindingToken = ",", bool i_typedColumns = true)
		// i_bindingToken = "," or "AND", etc.
		{
			JdFormatter f;
			
			i32 blobEnd = 0;
			
			// TODO: all these fundamentals probably don't need to be copied. like the unversioned obj & epigram, pointers to their
			// Epigram location can just be pushed to the blob vector... wasn't really thinking clearly when i wrote this originally i guess
			// OR, forgetting some gotcha now...
			
			i32 i = 0;
			for (auto & item : info)
			{
				std::string name = item.GetKeyString ();
				
				if (i_typedColumns)
				{
					name += "/";
					name += item.GetValueTypeChar ();

					if (item.IsObject () or item.IsType (c_jdTypeId::enumeration))
					{
						name += "/";
						name += item.GetClassName ();
					}
				}
				
				if (i > 0) f += i_bindingToken;
				f += "[", name, "]=";
				
				if (item.Count () > 1)
				{
					f += "?";
					
					auto payload = item.unsafePayload ();
					size_t size =  (u8 *) payload.second - (u8 *) payload.first;
					
					m_bindInfo.push_back ({ (u8 *) payload.first, (i32) size, 'B' });
				}
				else if (item.Is <i16> ())				{ f += (i16) item; }
				else if (item.Is <u16> ())				{ f += (u16) item; }
				else if (item.Is <i32> ())				{ f += (i32) item; }
				else if (item.Is <u32> ())				{ f += (u32) item; }
				else if (item.Is <i64> ())				{ f += (i64) item; }
				else if (item.Is <u64> ())				{ f += (i64) ((u64) item); }	// final cast to (i64) because SQL integers are signed 64.
				else if (item.Is <string> ())
				{
					f += "?";

					cstr_t str = item;
					size_t length = strlen (str) + 1;
					
					m_bindInfo.push_back ({ (voidptr_t) str, (i32) length, Jd::TypeIdChar <string> () });
				}
				else if (item.Is <f32> ())
				{
					f += "?";
					
					f64 v = item;
					
					m_bindInfo.push_back ({ .float64= v, 0, Jd::TypeIdChar <f64> () });
				}
				else if (item.Is <f64> ())
				{
					f += "?";
					
					f64 v = item;
					
					m_bindInfo.push_back ({ .float64= v, 0, Jd::TypeIdChar <f64> () });
				}
				else if (item.Is <u8> ())			{ f += (u32) ((u8) item); }
				else if (item.Is <i8> ())			{ f += (i32) ((i8) item); }
				else if (item.Is <EpBinary> ())
				{
					f += "?";
					
					EpBinary blob = item;

					m_bindInfo.push_back ({ (u8*) blob.GetData (), (i32) blob.GetSize(), 'B' });
				}
				else if (item.IsType (c_jdTypeId::uuid))
				{
					f += "?";
					auto ptr = item.unsafePointer <JdUUID> ();
					
					d_jdAssert (ptr, "raw epmsg obj was null");
					
					if (ptr)
					{
						m_bindInfo.push_back ({ (void *) ptr, sizeof (JdUUID), 'B' });
					}
				}
				else if (item.IsType (c_jdTypeId::enumeration))
				{
					f += "?";

					auto raw = item.GetRawObject ();
					
					i64 decoded = Jd::Decode7bRE <i64> (raw.data.start, raw.data.end);

					BindInfo bi;
					bi.int64 = decoded;
					bi.dataType = Jd::TypeIdChar <i64> ();
					
					m_bindInfo.push_back (bi);
				}
				else if (item.IsType (c_jdTypeId::object) or item.IsType (c_jdTypeId::pod))
				{
					f += "?";
					auto raw = item.GetRawObject ();
					
					d_jdAssert (raw.data.start, "raw epmsg obj was null");
					
					if (raw.data.start)
					{
						m_bindInfo.push_back ({ (void *) raw.data.start, (i32) raw.data.GetNumBytes (), 'B' });
					}
				}
				else if (item.IsType (c_jdTypeId::versionedObject))
				{
					f += "?";
					auto raw = item.GetRawObject ();
					
					d_jdAssert (raw.data.start, "raw epmsg obj was null");
					
					if (raw.data.start) // have to copy so that version byte can be inserted before
					{
						i32 requiredBytes = blobEnd + raw.data.GetNumBytes () + sizeof (raw.version);
                        
						if (requiredBytes > t_blobStorageSize)
							return std::string ();

						u8 *storage = & m_blobStorage [blobEnd];

						m_bindInfo.push_back ({ storage, (i32) raw.data.GetNumBytes () + 1, 'B' });
						
						*storage = raw.version;
						memcpy (++storage, raw.data.start, raw.data.GetNumBytes ());
						
						blobEnd = requiredBytes;
					}
				}
				else if (item.IsType (c_jdTypeId::epigram))
				{
					f += "?";
					auto raw = item.GetRawObject();
					
					d_jdAssert (raw.data.start, "raw epmsg msg was null");
					
					m_bindInfo.push_back ({ (voidptr_t) raw.data.start, (i32) raw.data.GetNumBytes (), 'B' });
				}
				
				++i;
			}
			
			m_bindInfo.push_back ({ & m_blobStorage [blobEnd], '*' });
			
			return f;
		}
		
		i32 /* o_nextBlobIndex */		BindBlobs					(IJdSql & i_sql, i32 i_blobStartIndex = 0)
		{
			for (int i = 0; i < m_bindInfo.size () - 1; ++i)
			{
				voidptr_t blobStart = m_bindInfo [i].data;
				
				char type = m_bindInfo [i].dataType;
				i32 length = m_bindInfo [i].length;
				
//				cout << length << endl;
				
				i32 result;
				if (type == 'B')
					result = i_sql->BindBlob (i_blobStartIndex + i, blobStart, length);
				else if (type == Jd::TypeIdChar <string> ())
					result = i_sql->BindString (i_blobStartIndex + i, (cstr_t) blobStart, length);
				else if (type == Jd::TypeIdChar <f32> ())
					result = i_sql->BindFloat (i_blobStartIndex + i, m_bindInfo [i].float64);
				else if (type == Jd::TypeIdChar <f64> ())
					result = i_sql->BindFloat (i_blobStartIndex + i, m_bindInfo [i].float64);
				else if (type == Jd::TypeIdChar <i64> ())
					result = i_sql->BindInt64 (i_blobStartIndex + i, m_bindInfo [i].int64);
				else
					throw ("unknown blob type\n");
			}
			
			return i_blobStartIndex + (i32) m_bindInfo.size () - 1;
		}
		
		struct alignas (8) BindInfo
		{
			union
			{
				const void *	data;
				f64				float64;
				i64				int64;
			};
			
			i32				length;
			char			dataType;
		};
        
		vector <BindInfo>			m_bindInfo;
		u8							m_blobStorage [t_blobStorageSize];
	};
    
//	std::string							CreateSqlColumnDescription	(Epigram25)
	
	JdResult							ModifyRow					(const rowid_t &i_uuid, EpigramRef i_columns)
	{
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
        JdResult result;
        
        jd_lock (jd_sql)
        {
			result = QuietSqlResult (ModifyRowInternal (i_uuid, i_columns));
        }
		
		return result;
	}
	
	
	protected://-------------------------------------------------------------------------------------------------------------------------------------------------

	JdResult						ModifyRowInternal				(const rowid_t &i_uuid, EpigramRef i_columns)
	{
		JdResult result;
		
		for (auto & item : i_columns)
		{
			string name = item.GetKeyString ();
			
			d_jdAssert (name.find ("/") == string::npos, "column names can't use a forward slash");
			
			if (not FindColumn (name.c_str()))
			{
				cstr_t sqlType = nullptr;
				
				name += "/";
				
//				if (item.count)
				
				name += item.GetValueTypeChar ();

				
					 if (item.Count () > 1)						{ sqlType = "BLOB"; }
				
				else if (item.Is <i64> ())						{ sqlType = "INTEGER"; }
				else if (item.Is <i32> ())						{ sqlType = "INTEGER"; }
				else if (item.Is <i16> ())						{ sqlType = "INTEGER"; }
				else if (item.Is <i8> ())						{ sqlType = "INTEGER"; }
				
				else if (item.Is <u64> ())						{ sqlType = "UNSIGNED INTEGER"; }
				else if (item.Is <u32> ())						{ sqlType = "UNSIGNED INTEGER"; }
				else if (item.Is <u16> ())						{ sqlType = "UNSIGNED INTEGER"; }
				else if (item.Is <u8> ())						{ sqlType = "UNSIGNED INTEGER"; }
				
				else if (item.Is <f32> ())						{ sqlType = "REAL"; }
				else if (item.Is <f64> ())						{ sqlType = "REAL"; }

				else if (item.Is <string> ())					{ sqlType = "TEXT"; }
				
				else if (item.Is <Epigram> ())					{ sqlType = "BLOB"; }
				
				else if (item.IsObject ())						{ sqlType = "BLOB"; name += "/"; name += item.GetClassName (); }
				else if (item.IsType (c_jdTypeId::enumeration))	{ sqlType = "UNSIGNED INTEGER"; name += "/"; name += item.GetClassName (); }
				else /* if (item.IsType (c_jdTypeId::uuid))	*/	{ sqlType = "BLOB"; }
				
				d_jdAssert (sqlType, "Epigram->SQL type mapping not defined");
				
				m_f << "ALTER TABLE ", m_databaseName, " ADD COLUMN [", name, "] ", sqlType;

//				cout << m_f << endl;

				result = jd_sql->Prepare (m_f);
				if (result) return result;
				
				result = jd_sql->Step ();
				if (result != c_jdSql::done) return result;
			}
		}
	
		m_f << "UPDATE ", m_databaseName, " SET seq=", IncrementSequenceNum (), ",";
	
		QueryGenerator <65536> generator;
		std::string columnSql = generator.EncodeColumns (i_columns, ",");
	
		m_f += columnSql, " WHERE ", m_rowIdName, "=?";
	
//			cout << m_f << endl;
	
		result = jd_sql->Prepare (m_f);
		if (not result)
		{
			i32 blobIndex = generator.BindBlobs (jd_sql);
		
			BindRowId (jd_sql, blobIndex, i_uuid);
		
			result = QuietSqlResult (jd_sql->Step ());
		}

		return result;
	}

	
	virtual i32						BindRowId						(IJdSql & i_sql, u32 i_queryIndex, const rowid_t &i_rowId) = 0;
	virtual rowid_t					GetRowIdFromColumn				(IJdSql & i_sql, u32 i_queryIndex) = 0;

	rows_t							ProcessQuery					()
	{        
        rows_t rows;

		// caller should lock
//        jd_lock (jd_sql)
        {
            JdResult result = jd_sql->Step ();
            
            i32 uuidIndex = -1;
            if (result == c_jdSql::rowReady)
            {
                uuidIndex = jd_sql->FindColumnIndex (m_rowIdName);
                
                d_jdAssert (uuidIndex >= 0, "couldn't find uuid column");
                
                while (result == c_jdSql::rowReady)
                {
//                    d_jdAssert (jd_sql->GetColumnNumBytes (uuidIndex) == 16, "uuid column has wrong # of bytes");
//                    
//                    const void * uuidBytes = jd_sql->GetColumnBlob (uuidIndex);
                    
//                    rowid_t uuid (uuidBytes);
					rowid_t uuid = GetRowIdFromColumn (jd_sql, uuidIndex);
                    
                    //cout << std::hex << uuidLow << " -- " << uuid.ToString () << endl;
                    rows.push_back (uuid);
                    
                    result = jd_sql->Step ();
                }
            }
        }
		return rows;
	}
    
	
	i32							FindColumn					(cstr_t i_columnName)
	{
        d_jdAssert (jd_sql, "sql interface not acquired.");
        
		SetupDatabase();
		d_jdAssert (m_tablesInitialized, "database wasn't initialized");
		
		u32 nameLength = (u32) strlen (i_columnName);
		
		m_f << "PRAGMA table_info (", m_databaseName, ")";		// this returns a table
		
		JdResult result = jd_sql->Prepare (m_f);
		if (not result)
		{
			int columnIndex = 0;
			
			result = jd_sql->Step ();
			
			while (result == c_jdSql::rowReady)
			{
				cstr_t sqlColumnName = jd_sql->GetColumnText (1);
				
				if (! sqlColumnName) break;
				
				cstr_t forwardSlash = strstr (sqlColumnName, "/");	// column names end in "/Type"
				
				if (forwardSlash)
				{
					u32 sqlColumnNameLength = (u32) (forwardSlash - sqlColumnName);
					
					if (sqlColumnNameLength == nameLength)
					{
						if (strncmp (i_columnName, sqlColumnName, nameLength) == 0)
							return columnIndex;
					}
				}
				
				result = jd_sql->Step ();
				++columnIndex;
			}
		}
		
		return 0;
	}
	
protected: //-------------------------------------------------------------------------------------------------------------------------------------------------------
	
//	void							QuietSqlResult				(JdResult & io_result)
//	{
//		if (io_result.GetCode () == SQLITE_DONE)
//			io_result = c_jdNoErr;
//	}
	
	JdResult						CreateTable					()
	{
		
	}
	
	JdResult 						SetupDatabase				()
	{
		JdResult result;

		if (m_tablesInitialized) return c_jdNoErr;
		
//		m_f << "CREATE TABLE IF NOT EXISTS ", m_databaseName, " (rowid INTEGER PRIMARY KEY, ";
//        m_f += m_rowIdName, " BLOB, ",
//		m_f += "seq UNSIGNED BIGINT, UNIQUE (";          // FIX: BLOB needs to shift with typename rowid_t
//        m_f += m_rowIdName, ") ON CONFLICT REPLACE)";

		m_f << "CREATE TABLE IF NOT EXISTS ", m_databaseName, " (rowid INTEGER PRIMARY KEY, seq UNSIGNED BIGINT";
		
		for (auto & c : m_definedColumns)
		{
			
		}
		m_definedColumns.clear ();
		
		if (m_columnDef.size())
		{
			m_f += ", ", m_columnDef;
			m_columnDef.clear ();
		}
		
        if (not m_useNativeSqliteRowId) m_f += ", ", m_rowIdName, " BLOB, UNIQUE (", m_rowIdName, ") ON CONFLICT REPLACE";
		
		m_f += ")";
		
//		cout << m_f << endl;

        jd_lock (jd_sql)
        {
            result = jd_sql->Prepare (m_f);
            
            if (result == c_jdNoErr)
                result = jd_sql->Step ();
            
            m_f << "SELECT * FROM ", m_databaseName, " ORDER BY seq DESC";
            
            result = jd_sql->Prepare (m_f);
            if (result == c_jdNoErr)
                result = jd_sql->Step ();
            
            if (result == c_jdSql::rowReady)
            {
//				cstr_t seqName = jd_sql->GetColumnName (1);				
				u64 sequence = jd_sql->GetColumn64 (1);
                if (sequence > * m_sequence) * m_sequence = sequence;
				epilog (normal, "table: @ has seq: @", m_databaseName, sequence);
                result = c_jdNoErr;
            }
            else if (result == c_jdSql::done)
                result = c_jdNoErr;
        }
        
        if (result == c_jdNoErr)
            m_tablesInitialized = true;
        
		if (result)
			epilog (normal, "@", result);

		return result;
	}
	
    IJdSql							jd_sql;
	
	struct ColumnInfo
	{
		u8			type;
		cstr_t		extended;
		string		name;
		u32			constraints;
	};
	
	string							m_columnDef;
	vector <ColumnInfo>				m_definedColumns;
    
	bool							m_tablesInitialized;
	JdString128						m_databaseName;

	cstr_t							m_rowIdName;
	bool							m_useNativeSqliteRowId		= false;
	i32								m_firstUserColumn;

	u64								m_localSequence				= 0;
	
	u64 *							m_sequence					= nullptr;
	
	JdFormatter						m_f;
	
	JdFormatter						m_query;
};


# if d_jdEnableEnvironment
template <typename T>
struct JdCoreDatabase : JdSqlTableT <T>, JdModuleHelper <IJdModuleServer>
{
	JdCoreDatabase					(cstr_t i_databaseName, cstr_t i_rowIdName, u64 * i_linkedSequence)
	:
	JdSqlTableT	<T>					(i_databaseName, i_rowIdName, i_linkedSequence)
	{ }
	
	JdResult						ConnectToPrimarySQL				(IJdModuleServer i_server)
	{
		JdResult result = this->jd_sql.Singleton (i_server);
		
		return result;
	}
	
	
	JdResult                        ConnectToSQL                    (IJdModuleServer i_server, stringRef_t i_instanceName, stringRef_t i_path,
																	 bool i_dropTableIfExtant = false)
	{
		JdResult result = this->jd_sql.Bind (i_server, i_instanceName, Epigram (a_sql::databasePath= i_path));
		
		if (not result)
		{
			if (i_dropTableIfExtant)
				this->DropTable ();
		}
		
		//		if (not result) result = SetupDatabase ();
		
		return result;
	}
	
	
	JdResult                        ConnectToSQL                    (IJdModuleServer i_server, bool i_dropTableIfExtant = false)
	{
		JdResult result = this->jd_sql.Bind (i_server, JdId ("shared")); //, & e);
		
		if (not result)
		{
			if (i_dropTableIfExtant)
			{
			}
		}
		
		return result;
	}

};

# else

#	define JdCoreDatabase JdSqlTableT

# endif


typedef JdCoreDatabase <JdUUID> JdDatabaseUUID;

class JdUuidSqlTable : public JdDatabaseUUID
{
    public:
	JdUuidSqlTable (cstr_t i_databaseName, u64 * i_linkedSequence = nullptr)
	:
	JdDatabaseUUID (i_databaseName, "uuid", i_linkedSequence)
	{
		this->m_useNativeSqliteRowId = false;
		// 0: (native) rowid, 1: seq, 2: rowId
		this->m_firstUserColumn = 3;
	}
	
	protected:
	
	virtual i32						BindRowId					(IJdSql & i_sql, u32 i_queryIndex, const JdUUID &i_rowId)
	{
		return i_sql->BindBlob (i_queryIndex, i_rowId.GetBytes(), 16);
	}

	virtual JdUUID					GetRowIdFromColumn			(IJdSql & i_sql, u32 i_columnIndex)
	{
		d_jdAssert (i_sql->GetColumnNumBytes (i_columnIndex) == 16, "uuid column has wrong # of bytes");
		
		const u8 * uuidBytes = (const u8 *) i_sql->GetColumnBlob (i_columnIndex);
		
		return JdUUID (uuidBytes);
	}
};


typedef JdCoreDatabase <i64> JdDatabase64;

class JdSqlTable : public JdDatabase64
{
	public:
	
	JdSqlTable (cstr_t i_tableName, u64 * i_linkedSequence = nullptr)
	:
	JdDatabase64 (i_tableName, "rowid", i_linkedSequence)
	{
		this->m_useNativeSqliteRowId = true;
		// 0: (native) rowid, 1: seq
		this->m_firstUserColumn = 2;
	}
	
	typedef i64 rowid_t;
	
	protected:
	
	i32								BindRowId					(IJdSql & i_sql, u32 i_queryIndex, const i64 &i_rowId)
	{
		return i_sql->BindInt64 (i_queryIndex, i_rowId);
	}

	virtual i64						GetRowIdFromColumn			(IJdSql & i_sql, u32 i_columnIndex)
	{
		return i_sql->GetColumn64 (i_columnIndex);
	}
};


//d_epilogCategory (database, JdDatabaseUUID, JdDatabase64)



