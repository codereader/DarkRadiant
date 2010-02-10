#include "FontLoader.h"

namespace readable
{
	FontInfo::FontInfo(float glyphScale, std::string name) : _glyphScale(glyphScale) 
	{
		_fontResolution = name.substr(name.length()-2,2);
	}

	FontInfoPtr FontLoader::decodeDat(Path FilePath)
	{
		Q3FontInfoPtr fontReader;
		FontInfoPtr ReturnFont;

		//Check file-extension. Append extension if necessary.
		if (FilePath.extension() == "")
		{
			std::string NewPath = FilePath.file_string();
			NewPath.append(".dat");
			FilePath = boost::filesystem::path(NewPath);
		}
		else if (FilePath.extension() != ".dat")
			reportError("[FontLoader::loadFont] File-extension is not \"dat\": " + FilePath.file_string() + "\n");

			//Check if file exists...
		if (boost::filesystem::exists(FilePath))
		{
			boost::filesystem::ifstream file(FilePath, std::ios_base::in | std::ios_base::binary);
			if (file.is_open() == false)
				reportError("[XDataManager::importXData] Failed to open file: " + FilePath.file_string() + "\n");

			//Check filesize...
			if (boost::filesystem::file_size(FilePath) == sizeof(q3font::fontInfo_t))
			{
				fontReader = Q3FontInfoPtr(new q3font::fontInfo_t);
				file.read((char*)&(*fontReader), sizeof(q3font::fontInfo_t));	//success? RETURNVALUE??
				file.close();
				globalOutputStream() << "FontLoader: "  << FilePath.file_string() << " loaded successfully." << std::endl;
				#ifdef printfDebug
					printf("dat loaded successfully...\n");
				#endif
			}
			else
				reportError("[FontLoader::loadFont] Filesize is not suitable for the q3fontInfo_t structure: " + FilePath.file_string() + "\n");
		}
		else
			reportError("[FontLoader::loadFont] File does not exist: " + FilePath.file_string() + "\n");

		//Store texturepath:
		ReturnFont = FontInfoPtr(new FontInfo(fontReader->glyphScale, fontReader->name) );
		std::string fontName = fontReader->glyphs[0].shaderName;
		fontName = fontName.substr(6,fontName.length()-15);					//dirty!!
		ReturnFont->_fontName = fontName;
		ReturnFont->_texturePath = "fonts/" + language + "/" + fontName + "/" + fontName;

		//Copy q3-struct to custom class.
		ReturnFont->_maxTextureIndex = 0;
		for (int n = 0; n<q3font::GlyphCountPerFont; n++)
		{
			//Possibly check necessary textures here as well later.
			if ( (fontReader->glyphs[n].s != 0) || (fontReader->glyphs[n].t != 0)) //Check if it is a valid glyph.  Not sure if this is the proper condition.
			{
				ReturnFont->_glyphs[n].reset(new GlyphInfo( fontReader->glyphs[n].height,
					fontReader->glyphs[n].top, fontReader->glyphs[n].bottom,
					fontReader->glyphs[n].pitch, fontReader->glyphs[n].xSkip,
					fontReader->glyphs[n].imageWidth, fontReader->glyphs[n].imageHeight,
					fontReader->glyphs[n].s, fontReader->glyphs[n].t,
					fontReader->glyphs[n].s2, fontReader->glyphs[n].t2,
					fontReader->glyphs[n].shaderName));
				//Check how many textures are used for this font.
				std::string checkTextureIndex = fontReader->glyphs[n].shaderName;
				checkTextureIndex = checkTextureIndex.substr(checkTextureIndex.length()-8,1);					//little dirty.
				if (ReturnFont->_maxTextureIndex < boost::lexical_cast<int>(checkTextureIndex) )
					ReturnFont->_maxTextureIndex = boost::lexical_cast<int>(checkTextureIndex);
			}
			else
				ReturnFont->_glyphs[n].reset();			
		}

		return ReturnFont;
	}


	void FontLoader::loadTextures()
	{	
		//Not working...
		if ( boost::filesystem::exists("dds/" + _font->_texturePath + "_0_" + _font->_fontResolution + ".dds") )
		{
			_textures.reset(new ImagePtr[_font->_maxTextureIndex+1]);
			//_textures = ImagePtr(new DDSImage[_font->_maxTextureIndex+1]);
			std::string pathHelper[2] = {"dds/" + _font->_texturePath + "_" , "_" + _font->_fontResolution + ".dds"};
			for (int n = 0; n <= _font->_maxTextureIndex; n++)
			{
				//_textures[n].reset ( _loaderDDS->load( *GlobalFileSystem().openFile( pathHelper[0] + boost::lexical_cast<std::string>(n) + pathHelper[1] ) ) );		//int to string!!
			}
		}
		else if ( boost::filesystem::exists(_font->_texturePath + "_0_" + _font->_fontResolution + ".tga") )
		{
			//Missing...
		}
		else
			reportError("[FontLoader::loadFont] Texture does not exist: " + _font->_texturePath + "\n");
	}

	FontLoader::FontLoader(std::string FileName)
	{
		_loaderTGA = GlobalImageLoader("TGA");
		_loaderDDS = GlobalImageLoader("DDS");
		
		loadFont(FileName);
	}

	FontLoader::FontLoader()
	{
		_loaderTGA = GlobalImageLoader("TGA");
		_loaderDDS = GlobalImageLoader("DDS");
	}

	void FontLoader::loadFont(std::string FileName)
	{
		Path Path_(FileName);
		_font = decodeDat(Path_);
		if (_font)
			loadTextures();
	}

	GlyphInfoPtr FontLoader::getGlyph(int ASCII_index)
	{
		if (_font->_glyphs[ASCII_index])	//check ASCII_index int conversion.
			throw std::runtime_error("[FontLoader::getGlyph] Glyph not available in this Font.");
		return _font->_glyphs[ASCII_index];
	}

	ImagePtr FontLoader::getTexture(int TextureIndex)
	{
		if (TextureIndex > _font->_maxTextureIndex)
			throw std::runtime_error("[FontLoader::getTexture] Requested TextureIndex is too high.");
		return _textures[TextureIndex];
	}

} // namespace readable


// temp backup


//readable::FontInfoPtr font = readable::DatDecoder::GetFont("fontImage_24.dat");

//ImageLoaderPtr LoaderTGA =  GlobalImageLoader("TGA");
//ImageLoaderPtr LoaderDDS = GlobalImageLoader("DDS");


/* Shared_ptr practice:
shared_ptr<int> a(new int(1));
shared_ptr<int> b = a;
*a += 6;
cout << *a << ", " << *b << endl;
a.reset();
cout << "a: " << (a ? "owns" : "empty") << endl;
cout << "b: " << (b ? "owns" : "empty") << endl;
cout << (a ? "a" : "X") << ", " << (b ? "b" : "X") << endl;

shared_ptr<int> owning(new int(47));
int * raw = owning.get();
cout << *raw << endl;
*raw = *owning;
cout << *raw << endl; //*/