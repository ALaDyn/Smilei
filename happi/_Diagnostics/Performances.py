from .Diagnostic import Diagnostic
from .._Utils import *


# Method to create a matrix containing the hindex of a 2D Hilbert curve
def HilbertCurveMatrix2D(m1, m2=None, oversize=0):
	import numpy as np
	if m2 is None: m2=m1
	o = oversize
	m = min(m1, m2)
	M = abs(m1-m2)
	A = np.empty((2**m2+2*o, 2**m1+2*o), dtype="uint32")
	A[o,o] = 0
	# For each Hilbert iteration
	for i in range(0,m):
		I = 2**i
		J = 2*I
		Npoints = I**2
		K1 = I + o
		K2 = J + o
		# Second quadrant is a transpose of the first
		A[o:K1 , K1:K2] = np.transpose(A[o:K1,o:K1])
		A[o:K1 , K1:K2] += Npoints
		Npoints *= 4
		# Second half is the same as the first half, but flipped
		A[K1:K2, o:K2] = (Npoints-1) - np.flipud(A[o:K1, o:K2])
		# For next iteration, the whole matrix is transposed
		A[o:K2, o:K2] = np.transpose(A[o:K2, o:K2])
	# Duplicate the Hilbert curve several times if non-square
	if m2>m1:
		A[o:K2, o:K2] = np.transpose(A[o:K2, o:K2])
		for j in range(1, 2**M):
			A[(j*J+o):((j+1)*J+o), o:K2] = A[o:K2, o:K2] + (j*Npoints)
	elif m1>m2:
		for j in range(1, 2**M):
			A[o:K2, (j*J+o):((j+1)*J+o)] = A[o:K2, o:K2] + (j*Npoints)
	return A

# Method to partition a matrix depending on a list of values
def PartitionMatrix( matrix, listOfValues, oversize=0 ):
	import numpy as np
	partitioned = np.empty(matrix.shape, dtype="uint32")
	max_index = 0
	for v in listOfValues:
		partitioned[matrix >= v] = max_index
		max_index += 1
	if oversize>0:
		partitioned[ :oversize,:] = np.uint32(-1)
		partitioned[:, :oversize] = np.uint32(-1)
		partitioned[-oversize:,:] = np.uint32(-1)
		partitioned[:,-oversize:] = np.uint32(-1)
	return partitioned


class Performances(Diagnostic):
	"""Class for loading a Performances diagnostic"""
	
	def _init(self, raw=None, map=None, histogram=None, timesteps=None, data_log=False, **kwargs):
		
		# Open the file(s) and load the data
		self._h5items = {}
		self._quantities = []
		for path in self._results_path:
			file = path+self._os.sep+'Performances.h5'
			try:
				f = self._h5py.File(file, 'r')
			except:
				self._error = "Diagnostic not loaded: Could not open '"+file+"'"
				return
			self._h5items.update( dict(f) )
			# Select only the fields that are common to all simulations
			values = f.values()
			if len(values)==0:
				self._quantities = []
			elif len(self._quantities)==0:
				self._quantities = list(next(iter(values)).keys())
			else:
				self._quantities = [f for f in next(iter(values)).keys() if f in self._quantities]
		# Converted to ordered list
		self._h5items = sorted(self._h5items.values(), key=lambda x:int(x.name[1:]))
		self._availableQuantities = list(self._quantities)
		
		nargs = (raw is not None) + (map is not None) + (histogram is not None)
		
		if nargs>1:
			self._error += "Diagnostic not loaded: choose only one of `raw`, `map` or `histogram`"
			return
		
		if nargs == 0:
			self._error += "Diagnostic not loaded: must define raw='quantity', map='quantity' or histogram=['quantity',min,max,nsteps]\n"
			self._error += "Available quantities: "+", ".join([str(q) for q in self._quantities])
			return
		
		# Get available times
		self._timesteps = self.getAvailableTimesteps()
		if self._timesteps.size == 0:
			self._error = "Diagnostic not loaded: No fields found"
			return
		
		# Get available quantities
		sortedquantities = reversed(sorted(self._quantities, key = len))
		
		# Get the number of procs of the data
		self._nprocs = self._h5items[0]["hindex"].shape[0]
		for item in self._h5items:
			if item["hindex"].shape[0] != self._nprocs:
				self._error = "Diagnostic not loaded: incompatible simulations"
		
		# Get the shape of patches
		self._number_of_patches = self.simulation.namelist.Main.number_of_patches
		self._tot_number_of_patches = self._np.prod( self._number_of_patches )
		
		# 1 - verifications, initialization
		# -------------------------------------------------------------------
		# Parse the `map` or `histogram` arguments
		if raw is not None:
			if type(raw) is not str:
				self._error += "Diagnostic not loaded: argument `raw` must be a string"
				return
			self.operation = raw
			self._mode = 0
			
		elif map is not None:
			if type(map) is not str:
				self._error += "Diagnostic not loaded: argument `map` must be a string"
				return
			if self._ndim > 2:
				self._error += "Diagnostic not loaded: argument `map` not available in "+str(self._ndim)+"D"
			self.operation = map
			self._mode = 1
			self._m = [int(self._np.log2(n)) for n in self._number_of_patches]
			
		elif histogram is not None:
			if type(histogram) is not list or len(histogram) != 4:
				self._error += "Diagnostic not loaded: argument `histogram` must be a list with 4 elements"
				return
			if type(histogram[0]) is not str:
				self._error += "Diagnostic not loaded: argument `histogram` must be a list with first element being a string"
				return
			self.operation = histogram[0]
			try:
				histogram_min    = float(histogram[1])
				histogram_max    = float(histogram[2])
				histogram_nsteps = int  (histogram[3])
			except:
				self._error += "Diagnostic not loaded: argument `histogram` must be a list like ['quantity',min,max,nsteps]"
				return
			self._mode = 2
		
		# Parse the operation
		self._operation = self.operation
		self._quantities = []
		for q in sortedquantities:
			if self._re.search(r"\b"+q+r"\b",self._operation):
				self._operation = self._re.sub(r"\b"+q+r"\b","C['"+q+"']",self._operation)
				self._quantities.append(q)
		
		# Build units
		units = {}
		for q in self._quantities:
			units.update({ q:{"t":"seconds", "h":"1", "n":"1"}[q[0]] })
		# Make total units and title
		self._operationunits = self.operation
		for q in self._quantities:
			self._operationunits = self._operationunits.replace(q, units[q])
		
		# Put data_log as object's variable
		self._data_log = data_log
		
		
		# 2 - Manage timesteps
		# -------------------------------------------------------------------
		# fill the "data" dictionary with indices to the data arrays
		self._data = {}
		for i,t in enumerate(self._timesteps):
			self._data.update({ t : i })
		# If timesteps is None, then keep all timesteps otherwise, select timesteps
		if timesteps is not None:
			try:
				self._timesteps = self._selectTimesteps(timesteps, self._timesteps)
			except:
				self._error = "Argument `timesteps` must be one or two non-negative integers"
				return
		
		# Need at least one timestep
		if self._timesteps.size < 1:
			self._error = "Timesteps not found"
			return
		
		
		# 3 - Manage axes
		# -------------------------------------------------------------------
		if raw is not None:
			self._type   .append("index of each process")
			self._shape  .append(self._nprocs)
			self._centers.append(self._np.arange(self._nprocs))
			self._label  .append("index of each process")
			self._units  .append("")
			self._log    .append(False)
			self._vunits = self._operationunits
			self._title  = self.operation
		
		elif map is not None:
			self._patch_length = [0]*self._ndim
			for iaxis in range(self._ndim):
				sim_length = self._ncels[iaxis]*self._cell_length[iaxis]
				self._patch_length[iaxis] = sim_length / float(self._number_of_patches[iaxis])
				centers = self._np.linspace(self._patch_length[iaxis]*0.5, sim_length-self._patch_length[iaxis]*0.5, self._number_of_patches[iaxis])
				self._type   .append("xyz"[iaxis])
				self._shape  .append(self._number_of_patches[iaxis])
				self._centers.append(centers)
				self._label  .append("xyz"[iaxis])
				self._units  .append("L_r")
				self._log    .append(False)
			self._vunits = self._operationunits
			self._title  = self.operation
		
		elif histogram is not None:
			bin_length = (histogram_max - histogram_min) / histogram_nsteps
			self._edges = self._np.linspace(histogram_min, histogram_max, histogram_nsteps+1)
			centers = self._edges[:-1] + bin_length * 0.5
			self._type   .append("quantity")
			self._shape  .append(histogram_nsteps)
			self._centers.append(centers)
			self._label  .append("quantity")
			self._units  .append("")
			self._log    .append(False)
			self._vunits = "1"
			self._title  = "number of processes"
		
		
		# Set the directory in case of exporting
		self._exportPrefix = "Performances_"+"".join(self._quantities)
		self._exportDir = self._setExportDir(self._exportPrefix)
		
		# Finish constructor
		self.valid = True
		return kwargs
	
	# Method to print info
	def _info(self):
		return "Performances diagnostic "+self._title
	
	# get all available timesteps
	def getAvailableTimesteps(self):
		try:    times = [float(a.name[1:]) for a in self._h5items]
		except: times = []
		return self._np.double(times)
	
	# get all available quantities
	def getAvailableQuantities(self):
		return self._availableQuantities
	
	# Method to obtain the data only
	def _getDataAtTime(self, t):
		if not self._validate(): return
		# Verify that the timestep is valid
		if t not in self._timesteps:
			print("Timestep "+str(t)+" not found in this diagnostic")
			return []
		# Get arrays from requested field
		# get data
		index = self._data[t]
		C = {}
		h5item = self._h5items[index]
		for q in self._quantities:
			C.update({ q : h5item[q].value })
		# Calculate the operation
		A = eval(self._operation)
		# log scale if requested
		if self._data_log: A = self._np.log10(A)
		
		# If raw requested
		if self._mode == 0:
			return A
		
		# If map requested
		elif self._mode == 1:
			if self._ndim == 1:
				# Make a matrix with MPI ranks at each patch location
				ranks = self._np.empty((self._number_of_patches[0],), dtype=self._np.uint32)
				hindices = iter(h5item["hindex"])
				previous_h = next(hindices)
				rank = 0
				for h in hindices:
					ranks[previous_h:h] = rank
					rank += 1
				ranks[h:] = rank
				
				# For each patch, associate the data of corresponding MPI rank
				return A[ranks]
			
			elif self._ndim == 2:
				# Make a matrix with patch indices on the Hilbert curve
				if not hasattr(self, "_hilbert"):
					self._hilbert = HilbertCurveMatrix2D(self._m[0], self._m[1], oversize=1)
				
				# Make a matrix with MPI ranks at each patch location
				self._ranks = PartitionMatrix( self._hilbert, h5item["hindex"], oversize=1 )
				
				# For each patch, associate the data of corresponding MPI rank
				return A[self._ranks[1:-1,1:-1]]
		
		# If histogram requested
		elif self._mode == 2:
			histogram, _ = self._np.histogram( A, self._edges )
			return histogram
	
	
	def _prepare4(self):
		if self._mode == 1 and self._ndim==2:
			self._extent = [
				0., self._xfactor*self._number_of_patches[0]*self._patch_length[0],
				0., self._yfactor*self._number_of_patches[1]*self._patch_length[1]
			]
	
	
	def _animateOnAxes_2D_(self, ax, A):
		# Display the data
		im = ax.imshow( self._np.flipud(A),
			vmin = self.options.vmin, vmax = self.options.vmax, extent=self._extent, **self.options.image)
		
		# Add lines to visualize MPI contours
		# Vertical lines
		vlines_i    = []
		vlines_jmin = []
		vlines_jmax = []
		vdiff = (self._np.diff(self._ranks, axis=1) != 0)
		vindices = self._np.flatnonzero(self._np.any(vdiff, axis=0)) # i-indices where vertical lines occur
		for i in vindices:
			j = self._np.flatnonzero(self._np.diff(vdiff[:,i])) # starts and ends of vertical lines
			vlines_i += [ self._np.full((len(j)/2), i, dtype=self._np.uint32) ]
			vlines_jmin += [ j[ 0::2 ] ]
			vlines_jmax += [ j[ 1::2 ] ]
		vlines_i    = self._np.concatenate( vlines_i    )*self._xfactor*self._patch_length[0]
		vlines_jmin = self._np.concatenate( vlines_jmin )*self._yfactor*self._patch_length[1]
		vlines_jmax = self._np.concatenate( vlines_jmax )*self._yfactor*self._patch_length[1]
		ax.vlines( vlines_i, vlines_jmin, vlines_jmax, **self.options.plot)
		
		# Horizontal lines
		hlines_j    = []
		hlines_imin = []
		hlines_imax = []
		hdiff = (self._np.diff(self._ranks, axis=0) != 0)
		hindices = self._np.flatnonzero(self._np.any(hdiff, axis=1)) # j-indices where horizontal lines occur
		for j in hindices:
			i = self._np.flatnonzero(self._np.diff(hdiff[j,:])) # starts and ends of horizontal lines
			hlines_j += [ self._np.full((len(i)/2), j, dtype=self._np.uint32) ]
			hlines_imin += [ i[ 0::2 ] ]
			hlines_imax += [ i[ 1::2 ] ]
		hlines_j    = self._np.concatenate( hlines_j    )*self._yfactor*self._patch_length[1]
		hlines_imin = self._np.concatenate( hlines_imin )*self._xfactor*self._patch_length[0]
		hlines_imax = self._np.concatenate( hlines_imax )*self._xfactor*self._patch_length[0]
		ax.hlines( hlines_j, hlines_imin, hlines_imax, **self.options.plot)
		
		return im
