
#include <string>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <set>

namespace zipr
{
	using namespace std;

	using ZiprOptionType_t = enum ZiprOptionType 
		{
			zotZiprUntypedOptionType,
			zotZiprIntegerOptionType,
			zotZiprDoubleOptionType,
			zotZiprBooleanOptionType,
			zotZiprStringOptionType,
		};

	class ZiprOption_t : virtual public Zipr_SDK::ZiprOption_t
	{
		public:
			ZiprOption_t(ZiprOptionType_t type, const string& key, const string& value)
				: m_key(key),
				  m_untyped_value(value),
					m_observee(nullptr),
					m_takes_value(true),
					m_needs_value(true),
					m_required(false),
					m_set(false),
					m_option_type(type) {};
			virtual ~ZiprOption_t() { } 

			virtual void setValue(const string &value) = 0;
			virtual void setOption() = 0;

			string getStringValue()  const
			{
				return m_untyped_value;
			}
			virtual string getValueString()  const override
			{
				return m_untyped_value;
			}
			string getNamespace()  const override
			{
				return m_namespace;
			}
			string getKey()  const override
			{
				return m_key;
			}
			void setNeedsValue(bool needs_value) 
			{
				m_needs_value = needs_value;
			}
			bool getNeedsValue()  const
			{
				return m_needs_value;
			}
			void setTakesValue(bool takes_value) 
			{
				m_takes_value = takes_value;
			}
			bool getTakesValue()  const
			{
				return m_takes_value;
			}
			
			bool isRequired()  const
			{
				return m_required;
			}
			void setRequired(bool required=true) 
			{
				m_required = required;
			}
			bool areRequirementMet()  const
			{
				return (!m_required) || (m_set);
			}

			virtual string getDescription()  const override
			{
				return m_description;
			}
			

			void setDescription(string description) 
			{
				m_description = description;
			}

			void addObserver(ZiprOption_t *observer) 
			{
				assert(observer->m_option_type == m_option_type);
				//observer->setValue(m_untyped_value);
				observer->m_observee = this;
				m_observers.insert(observer);
			}
		protected:
			string m_namespace, m_key, m_untyped_value, m_description;
			set<ZiprOption_t*> m_observers;
			ZiprOption_t *m_observee;
			bool m_takes_value, m_needs_value;
			bool m_required;
			bool m_set;
			void SetObserverValues(string value) 
			{
				set<ZiprOption_t*>::const_iterator it = m_observers.begin();
				set<ZiprOption_t*>::const_iterator it_end = m_observers.end();
				for (; it!=it_end; it++)
					(*it)->setValue(value);
			}
			ZiprOptionType_t m_option_type;
	};

	template <class T>
	class ZiprTypedOption_t : virtual public Zipr_SDK::ZiprTypedOption_t<T>,  public ZiprOption_t  
	{
		public:
			ZiprTypedOption_t(ZiprOptionType_t type,
				const string& key,
				const string& value)
				: ZiprOption_t(type, key, value) {}

			void setValue(const string &value) 
			{
				m_set = true;
				m_untyped_value = value;
				m_value = convertToTyped(value);
			}
			virtual void setOption() 
			{
			}

			T getValue() const 
			{
				if (m_observee!=nullptr)
				
				{
					auto typed_observee = dynamic_cast<Zipr_SDK::ZiprTypedOption_t<T>*>(m_observee);
					return typed_observee->getValue();
				}
				return m_value;
			}
			operator T() const 
			{
				return getValue();
			};
			virtual string getDescription() 
			{
				return string(m_key + ": " 
					+ ((!m_needs_value)?"[":"")
					+ "<" + getTypeName() + ">"
					+ ((!m_needs_value)?"]":"")
					+ ((m_description.length())?":\t":"") + m_description 
					);
			}
		protected:
			T m_value;
			virtual T convertToTyped(const string& ) = 0;
			virtual string convertToUntyped(const T&) = 0;
			virtual string getTypeName() = 0;
	};

	template <class T>
	class ZiprCompositeOption_t : public ZiprTypedOption_t<T>, public T
	{
		public:
			ZiprCompositeOption_t(ZiprOptionType_t type,
				string key,
				string value)
				: ZiprTypedOption_t<T>(type, key, value) {}
			using ZiprTypedOption_t<T>::convertToTyped;
			void setValue(const string &value) 
			{
				ZiprTypedOption_t<T>::setValue(value);
				T::operator=(convertToTyped(value));
			}
	};

	class ZiprStringOption_t : virtual public Zipr_SDK::ZiprStringOption_t, public ZiprCompositeOption_t<string>
	{
		public:
			ZiprStringOption_t(string key, string value = "")
				: ZiprCompositeOption_t(zotZiprStringOptionType, key, value) 
			{
				ZiprCompositeOption_t::setValue(value);
			}
			void setValue(const string& value) 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}
		protected:
			string convertToTyped(const string& value_to_convert) 
			{
				return string(value_to_convert);
			}
			string convertToUntyped(const string& value_to_convert) 
			{
				return string(value_to_convert);
			}
			string getTypeName() 
			{
				return "string";
			}
	};

	class ZiprBooleanOption_t : virtual public Zipr_SDK::ZiprBooleanOption_t, public ZiprTypedOption_t<bool>
	{
		public:
			ZiprBooleanOption_t(const string& key, const string& value = "")
				: ZiprTypedOption_t(zotZiprBooleanOptionType, key, value) 
			{
				m_untyped_value = value;
				m_needs_value = false;
			}
			ZiprBooleanOption_t(const string& key, bool value)
				: ZiprTypedOption_t(zotZiprBooleanOptionType, key, "") 
			{
				m_value = value;
				m_needs_value = false;
				m_untyped_value = convertToUntyped(value);
			}
			void setOption() 
			{
				m_untyped_value = "true";
				m_value = true;
				m_set = true;
			}
			void setValue(const bool& value) 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}

		private:
			bool convertToTyped(const string& value_to_convert) 
			{
				if (value_to_convert == "true")
					return true;
				else
					return false;
			}
			string convertToUntyped(const bool& value_to_convert) 
			{
				return (value_to_convert ? string("true") : string("false"));
			}
			string getTypeName() 
			{
				return "bool";
			}
	};

	class ZiprIntegerOption_t : virtual public Zipr_SDK::ZiprIntegerOption_t, public ZiprTypedOption_t<size_t>
	{
		public:
			ZiprIntegerOption_t(const string& key, const string& value = "")
				: zipr::ZiprTypedOption_t<size_t>(zotZiprIntegerOptionType, key, value) 
			{
				m_value = convertToTyped(value);
			}
			ZiprIntegerOption_t(const string& key, size_t value)
				: ZiprTypedOption_t<size_t>(zotZiprIntegerOptionType, key, "") 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}
			void setValue(const size_t& value) 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}
		private:
			size_t convertToTyped(const string& value_to_convert) 
			{
				int converted = 0;
				char *endptr = nullptr;
				converted = strtol(value_to_convert.c_str(),
						   &endptr, 10);
				if (*endptr != '\0')
				{
					m_set = false;
					m_untyped_value = "";
					return 0;
				}
				return converted;
			}
			string convertToUntyped(const size_t& value_to_convert) 
			{
				stringstream ss;
				ss << value_to_convert;
				return ss.str();
			}
			string getTypeName() 
			{
				return "integer";
			}
	};

	class ZiprDoubleOption_t : virtual public Zipr_SDK::ZiprDoubleOption_t, public ZiprTypedOption_t<double>
	{
		public:
			ZiprDoubleOption_t(const string& key, const string& value = "")
				: ZiprTypedOption_t<double>(zotZiprDoubleOptionType, key, value) 
				{
				m_value = convertToTyped(value);
			}
			ZiprDoubleOption_t(const string& key, double value)
				: ZiprTypedOption_t<double>(zotZiprDoubleOptionType, key, "") 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}
			void setValue(const double& value) 
			{
				m_value = value;
				m_untyped_value = convertToUntyped(value);
			}
			double getValue() const 
			{
				if (m_observee!=nullptr)
				
				{
					auto typed_observee = dynamic_cast<Zipr_SDK::ZiprTypedOption_t<double>*>(m_observee);
					return typed_observee->getValue();
				}
				return m_value;
			}
			operator double() const 
			{
				return getValue();
			};
		private:
			double convertToTyped(const string& value_to_convert) 
			{
				double converted = 0.;
				char *endptr = nullptr;
				converted = strtof(value_to_convert.c_str(),
						   &endptr);
				if (*endptr != '\0')
				{
					m_set = false;
					m_untyped_value = "";
					return 0;
				}
				return converted;
			}
			string convertToUntyped(const double& value_to_convert) 
			{
				stringstream ss;
				ss << value_to_convert;
				return ss.str();
			}
			string getTypeName() {
				return "double";
			}
	};

	class ZiprOptionsNamespace_t : public Zipr_SDK::ZiprOptionsNamespace_t, public set<ZiprOption_t*>
	{
		public:
			ZiprOptionsNamespace_t(const string& ns) : m_namespace(ns) {}
			virtual ~ZiprOptionsNamespace_t()
			{
				for(auto opt : all_options )
					delete opt;
			}

			string getNamespace() 
			{
				return m_namespace;
			};
			void printNamespace();
			ZiprOption_t *optionByKey(const string& key);
			void addOption(ZiprOption_t *option);
			void printUsage(int tabs, ostream &out);
			bool areRequirementsMet() const ;
			void mergeNamespace(ZiprOptionsNamespace_t*);

                        virtual string getNamespaceString() const override { return m_namespace; } 


                        virtual Zipr_SDK::ZiprStringOption_t*  getStringOption (const string& name, const string &description="", const string& default_value="")    override 
				{ return getOption<zipr::ZiprStringOption_t, string>(name,description,default_value); }
                        virtual Zipr_SDK::ZiprIntegerOption_t* getIntegerOption(const string& name, const string &description="", const size_t& default_value=0)     override 
				{ return getOption<zipr::ZiprIntegerOption_t, size_t>(name,description,default_value); }
                        virtual Zipr_SDK::ZiprBooleanOption_t* getBooleanOption(const string& name, const string &description="", const bool  & default_value=false) override 
				{ return getOption<zipr::ZiprBooleanOption_t, bool>(name,description,default_value); }
                        virtual Zipr_SDK::ZiprDoubleOption_t*  getDoubleOption (const string& name, const string &description="", const double& default_value=0.0)   override
				{ return getOption<zipr::ZiprDoubleOption_t, double>(name,description,default_value); }


		private:
			string m_namespace;
			set<ZiprOption_t*> all_options;

			template<class S, class T>
			S* getOption(const string& name, const string &description, const T& default_value)
			{
				auto new_opt=new S(name,default_value);
				assert(new_opt);
				new_opt->setDescription(description);
				all_options.insert(new_opt);
				return new_opt;
			}

	};

	class ZiprOptions_t  : public Zipr_SDK::ZiprOptionsManager_t
	{
		public:
			ZiprOptions_t(int argc, char **argv);
			~ZiprOptions_t() 
			{
				for(auto ns : m_namespaces)
					delete ns;
			}

			void addNamespace(ZiprOptionsNamespace_t *);
			void printNamespaces();
			bool parse(ostream *error=&cout, ostream *warn=&cerr);
			Zipr_SDK::ZiprOptionsNamespace_t *getNamespace(const string& name);
			void printUsage(ostream &out);
			bool areRequirementsMet() const;


		private:
			vector<string> m_arguments;
			set<ZiprOptionsNamespace_t*> m_namespaces;
	};

}
