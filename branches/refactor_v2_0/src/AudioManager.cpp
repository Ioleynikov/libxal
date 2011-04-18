/// @file
/// @author  Kresimir Spes
/// @author  Boris Mikic
/// @author  Ivan Vucica
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#include <hltypes/exception.h>
#include <hltypes/hdir.h>
#include <hltypes/hmap.h>
#include <hltypes/hstring.h>
#include <hltypes/hthread.h>
#include <hltypes/util.h>

#include "AudioManager.h"
#include "Category.h"
#include "Player.h"
#include "Sound.h"
#include "xal.h"

namespace xal
{
	AudioManager* mgr;

	AudioManager::AudioManager(chstr deviceName, bool threaded, float updateTime) : enabled(false),
		gain(1.0f), updating(false), thread(NULL)
	{
		this->deviceName = deviceName;
		this->updateTime = updateTime;
	}

	AudioManager::~AudioManager()
	{
		if (this->thread != NULL)
		{
			while (this->updating);
			this->thread->stop();
			delete this->thread;
		}
		foreach (Player*, it, this->players)
		{
			(*it)->stop();
			delete (*it);
		}
		this->players.clear();
		foreach_m (Sound*, it, this->sounds)
		{
			delete it->second;
		}
		this->sounds.clear();
		foreach_m (Category*, it, this->categories)
		{
			delete it->second;
		}
		this->categories.clear();
	}
	
	void AudioManager::_setupThread()
	{
		xal::log("starting thread management");
		this->updateTime = updateTime;
		this->updating = true;
		this->thread = new hthread(&AudioManager::update);
		this->thread->start();
		this->updating = false;
	}

	void AudioManager::setGlobalGain(float value)
	{
		this->gain = value;
		foreach (Player*, it, this->players)
		{
			(*it)->setGain((*it)->getGain());
		}
	}

	void AudioManager::update()
	{
		while (true)
		{
			while (xal::mgr->updating);
			xal::mgr->updating = true;
			xal::mgr->update(xal::mgr->updateTime);
			xal::mgr->updating = false;
			hthread::sleep(xal::mgr->updateTime * 1000);
		}
	}
	
	void AudioManager::update(float k)
	{
		if (this->isEnabled())
		{
			foreach (Player*, it, this->players)
			{
				(*it)->update(k);
			}
			harray<Player*> players(this->managedPlayers);
			foreach (Player*, it, players)
			{
				if (!(*it)->isPlaying() && !(*it)->isFading())
				{
					this->_destroyManagedPlayer(*it);
				}
			}
		}
	}

	void AudioManager::lockUpdate()
	{
		while (this->updating);
		this->updating = true;
	}
	
	void AudioManager::unlockUpdate()
	{
		this->updating = false;
	}

	Category* AudioManager::createCategory(chstr name, bool streamed, bool dynamicLoad)
	{
		if (!this->categories.has_key(name))
		{
			this->categories[name] = new Category(name, streamed, dynamicLoad);
		}
		return this->categories[name];
	}

	Category* AudioManager::getCategoryByName(chstr name)
	{
		if (!this->categories.has_key(name))
		{
			throw ("Audio Manager: Category '" + name + "' does not exist!").c_str();
		}
		return this->categories[name];
	}

	float AudioManager::getCategoryGain(chstr name)
	{
		if (!this->categories.has_key(name))
		{
			throw ("Audio Manager: Category '" + name + "' does not exist!").c_str();
		}
		return this->categories[name]->getGain();
	}

	void AudioManager::setCategoryGain(chstr name, float gain)
	{
		this->getCategoryByName(name)->setGain(gain);
		foreach (Player*, it, this->players)
		{
			(*it)->setGain((*it)->getGain());
		}
	}

	Sound* AudioManager::createSound(chstr filename, chstr categoryName, chstr prefix)
	{
		Category* category = this->getCategoryByName(categoryName);
		Sound* sound = new Sound(filename, category, prefix);
		// TODO
		/*
		///##
		if (category->isDynamicLoad())
		{
			xal::log("created a dynamic sound: " + filename);
		}
		else if (!sound->load())
		{
			xal::log("failed to load sound " + filename);
			return NULL;
		}
		*/
		this->sounds[sound->getName()] = sound;
		return sound;
	}

	harray<hstr> AudioManager::createSoundsFromPath(chstr path, chstr prefix)
	{
		harray<hstr> result;
		hstr category;
		harray<hstr> dirs = hdir::directories(path, true);
		foreach (hstr, it, dirs)
		{
			category = (*it).rsplit("/").pop_back();
			result += this->createSoundsFromPath((*it).c_str(), category, prefix);
		}
		return result;
	}

	harray<hstr> AudioManager::createSoundsFromPath(chstr path, chstr category, chstr prefix)
	{
		this->createCategory(category);
		harray<hstr> result;
		harray<hstr> files = hdir::files(path, true);
		Sound* sound;
		foreach (hstr, it, files)
		{
			sound = this->createSound((*it), category, prefix);
			if (sound != NULL)
			{
				result += sound->getName();
			}
		}
		return result;
	}

	void AudioManager::destroySound(Sound* sound)
	{
		foreach_m (Sound*, it, this->sounds)
		{
			if (it->second == sound)
			{
				delete it->second;
				this->sounds.erase(it);
				break;
			}
		}
	}
	
	void AudioManager::destroySoundsWithPrefix(chstr prefix)
	{
		harray<hstr> keys = this->sounds.keys();
		foreach (hstr, it, keys)
		{
			if ((*it).starts_with(prefix))
			{
				delete this->sounds[*it];
				this->sounds.remove_key(*it);
			}
		}
	}

	Player* AudioManager::createPlayer(chstr name)
	{
		Sound* sound = this->sounds[name];
		Player* player = this->_createAudioPlayer(this->sounds[name], this->sounds[name]->getBuffer());
		this->players += player;
		return player;
	}

	void AudioManager::destroyPlayer(Player* player)
	{
		this->players -= player;
		player->stop();
		delete player;
	}

	Player* AudioManager::_createManagedPlayer(chstr name)
	{
		Player* player = this->createPlayer(name);
		this->managedPlayers += player;
		return player;
	}

	void AudioManager::_destroyManagedPlayer(Player* player)
	{
		this->managedPlayers -= player;
		this->destroyPlayer(player);
	}

	Player* AudioManager::_createAudioPlayer(Sound* sound, Buffer* buffer)
	{
		return new Player(sound, buffer);
	}
	
	void AudioManager::play(chstr name, float fadeTime, bool looping, float gain)
	{
		Player* player = this->_createManagedPlayer(name);
		player->setGain(gain);
		player->play(fadeTime, looping);
	}

	void AudioManager::stop(chstr name, float fadeTime)
	{
		this->lockUpdate();
		fadeTime = hmax(fadeTime, 0.0f);
		harray<Player*> players;
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name)
			{
				if (fadeTime == 0.0f)
				{
					players += (*it);
				}
				else
				{
					(*it)->stop(fadeTime);
				}
			}
		}
		foreach (Player*, it, players)
		{
			this->_destroyManagedPlayer(*it);
		}
		this->unlockUpdate();
	}

	void AudioManager::stopFirst(chstr name, float fadeTime)
	{
		this->lockUpdate();
		fadeTime = hmax(fadeTime, 0.0f);
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name)
			{
				if (fadeTime == 0.0f)
				{
					this->_destroyManagedPlayer(*it);
				}
				else
				{
					(*it)->stop(fadeTime);
				}
				break;
			}
		}
		this->unlockUpdate();
	}

	void AudioManager::stopAll(float fadeTime)
	{
		this->lockUpdate();
		fadeTime = hmax(fadeTime, 0.0f);
		if (fadeTime == 0.0f)
		{
			harray<Player*> players(this->managedPlayers);
			foreach (Player*, it, players)
			{
				this->_destroyManagedPlayer(*it);
			}
			foreach (Player*, it, this->players)
			{
				(*it)->stop();
			}
		}
		else
		{
			foreach (Player*, it, this->players)
			{
				(*it)->stop(fadeTime);
			}
		}
		this->unlockUpdate();
	}
	
	void AudioManager::stopCategory(chstr name, float fadeTime)
	{
		this->lockUpdate();
		fadeTime = hmax(fadeTime, 0.0f);
		Category* category = this->getCategoryByName(name);
		if (fadeTime == 0.0f)
		{
			harray<Player*> players(this->managedPlayers);
			foreach (Player*, it, players)
			{
				if ((*it)->getCategory() == category)
				{
					this->_destroyManagedPlayer(*it);
				}
			}
			foreach (Player*, it, this->players)
			{
				if ((*it)->getCategory() == category)
				{
					(*it)->stop();
				}
			}
		}
		else
		{
			foreach (Player*, it, this->players)
			{
				if ((*it)->getCategory() == category)
				{
					(*it)->stop(fadeTime);
				}
			}
		}
		this->unlockUpdate();
	}
	
	bool AudioManager::isAnyPlaying(chstr name)
	{
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name && (*it)->isPlaying())
			{
				return true;
			}
		}
		return false;
	}

	bool AudioManager::isAnyFading(chstr name)
	{
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name && (*it)->isFading())
			{
				return true;
			}
		}
		return false;
	}

	bool AudioManager::isAnyFadingIn(chstr name)
	{
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name && (*it)->isFadingIn())
			{
				return true;
			}
		}
		return false;
	}

	bool AudioManager::isAnyFadingOut(chstr name)
	{
		foreach (Player*, it, this->managedPlayers)
		{
			if ((*it)->getSound()->getName() == name && (*it)->isFadingOut())
			{
				return true;
			}
		}
		return false;
	}

}