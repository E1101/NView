<?php

abstract class AbstractEnvironment implements EnvironmentInterface {
	protected $env = array();
	protected $ienv = array();

	public function initialize(array $env = array()) {
		$this->env = $env;
		$this->ienv = array_change_key_case($env, \CASE_LOWER);
	}

	public function initializeByRef(array &$env = array()) {
		$this->initialize($env);
		$this->env = &$env;
	}

	public function all() {
		return $this->env;
	}

	/**
	 * Removes an attribute.
	 *
	 * @param string $name
	 *
	 * @return mixed The removed value or null when it does not exist
	 */
	public function remove($name) {
		$retval = null;
		if (array_key_exists($name, $this->env)) {
			$retval = $this->env[$name];
			unset($this->env[$name]);
		}
		return $retval;
	}

	/**
	 * Clears out data from bag.
	 *
	 * @return mixed Whatever data was contained.
	 */
	public function clear(){
		$retval = $this->env;
		$this->env = array();
		return $retval;
	}

	public function has($v) {
		$v = is_array($v) ? $v : func_get_args();
		return $this->doHas($v, $this->env);
	}

	public function ihas($v) {
		$v = is_array($v) ? $v : func_get_args();
		$v = array_map('strtolower', $v);
		return $this->doHas($v, $this->ienv);
	}

	public function set($key, $value) {
		$this->env[$key] = $value;
	}

	public function get($v, $d = null) {
		if ($this->has($v)) {
			return $this->env[$v];
		} else {
			return $d;
		}
	}

	public function iget($v, $d = null) {
		if ($this->ihas($v)) {
			return $this->ienv[strtolower($v)];
		} else {
			return $d;
		}
	}

	public function sig($v) {
		$v = is_array($v) ? $v : func_get_args();
		return $this->doSig($v, $this->env);
	}

	public function isig($v) {
		$v = is_array($v) ? $v : func_get_args();
		$v = array_map('strtolower', $v);
		return $this->doSig($v, $this->ienv);
	}

	private function doSig(array $keys, array $repo) {
		foreach ($keys as $key) {
			if (!isset($repo[$key]) || $this->isEmptyString($repo[$key])) {
				return false;
			}
		}
		return true;
	}

	private function doHas(array $keys, array $repo) {
		foreach ($keys as $key) {
			if (!isset($repo[$key])) {
				return false;
			}
		}
		return true;
	}

	private function isEmptyString($value) {
		$boolOrArray = is_bool($value) || is_array($value);
		return !$boolOrArray && trim((string)$value) === '';
	}
}

