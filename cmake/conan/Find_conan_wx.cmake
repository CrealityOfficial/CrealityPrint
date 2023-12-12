# wx target

set(WX_COMPONETS wx_adv
				 wx_aui
				 wx_base
				 wx_core
				 wx_gl
				 wx_html
				 wx_media
				 wx_net
				 wx_propgrid
				 wx_qa
				 wx_ribbon
				 wx_richtext
				 wx_stc
				 wx_webview
				 wx_xml
				 wx_xrc
				 )
					
__conan_import(wx dll COMPONENT ${WX_COMPONETS})
