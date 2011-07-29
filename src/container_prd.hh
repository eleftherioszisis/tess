// Voro++, a 3D cell-based Voronoi library
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : May 18th 2011

/** \file container.hh
 * \brief Header file for the container_base template and related classes. */

#ifndef VOROPP_CONTAINER_HH
#define VOROPP_CONTAINER_HH

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
using namespace std;

#include "config.hh"
#include "common.hh"
#include "v_base.hh"
#include "cell.hh"
#include "v_loops.hh"
#include "v_compute.hh"
#include "unitcell.hh"

class container_periodic_base : public unitcell, public voropp_base {
	public:
		/** The maximum x coordinate of the container. */
		const double bx;
		const double bxy;
		/** The maximum y coordinate of the container. */
		const double by;
		const double bxz;
		const double byz;
		/** The maximum z coordinate of the container. */
		const double bz;
		/** This array holds the numerical IDs of each particle in each
		 * computational box. */
		int **id;
		/** A two dimensional array holding particle positions. For the
		 * derived container_poly class, this also holds particle
		 * radii. */
		double **p;
		/** This array holds the number of particles within each
		 * computational box of the container. */
		int *co;
		/** This array holds the maximum amount of particle memory for
		 * each computational box of the container. If the number of
		 * particles in a particular box ever approaches this limit,
		 * more is allocated using the add_particle_memory() function.
		 */
		int *mem;
		/** The amount of memory in the array structure for each
		 * particle. This is set to 3 when the basic class is
		 * initialized, so that the array holds (x,y,z) positions. If
		 * the container class is initialized as part of the derived
		 * class container_poly, then this is set to 4, to also hold
		 * the particle radii. */
		const int ps;
		container_periodic_base(double bx_,double bxy_,double by_,double bxz_,double byz_,double bz_,
				int nx_,int ny_,int nz_,int init_mem,int ps);
		~container_periodic_base();
		bool point_inside(double x,double y,double z);
		void region_count();
		/** Initialize the Voronoi cell to be the entire container. For
		 * non-periodic coordinates, this is set by the position of the
		 * walls. For periodic coordinates, the space is equally
		 * divided in either direction from the particle's initial
		 * position. That makes sense since those boundaries would be
		 * made by the neighboring periodic images of this particle. It
		 * also applies plane cuts made by any walls that have been
		 * added to the container.
		 * \param[in,out] c a reference to a voronoicell object.
		 * \param[in] (x,y,z) the position of the particle.
		 * \return False if the plane cuts applied by walls completely
		 * removed the cell, true otherwise. */
		template<class v_cell>
		inline bool initialize_voronoicell(v_cell &c,int ijk,int q,int ci,int cj,int ck,int &i,int &j,int &k,double &x,double &y,double &z,int &disp) {
			double x1,x2,y1,y2,z1,z2;
			double *pp(p[ijk]+ps*q);
			x=*(pp++);y=*(pp++);z=*pp;
			if(xperiodic) {x1=-(x2=0.5*(bx-ax));i=nx;} else {x1=ax-x;x2=bx-x;i=ci;}
			if(yperiodic) {y1=-(y2=0.5*(by-ay));j=ny;} else {y1=ay-y;y2=by-y;j=cj;}
			if(zperiodic) {z1=-(z2=0.5*(bz-az));k=nz;} else {z1=az-z;z2=bz-z;k=ck;}
			c.init(x1,x2,y1,y2,z1,z2);
			if(!apply_walls(c,x,y,z)) return false;
			disp=ijk-i-nx*(j+ny*k);
			return true;
		}
		/** Returns the position of a particle currently being computed
		 * relative to the computational block that it is within. It is
		 * used to select the optimal worklist entry to use.
		 * \param[in] (x,y,z) the position of the particle.
		 * \param[in] (ci,cj,ck) the block that the particle is within.
		 * \param[out] (fx,fy,fz) the position relateive to the block.
		 */
		inline void frac_pos(double x,double y,double z,double ci,double cj,double ck,double &fx,double &fy,double &fz) {
			fx=x-ax-boxx*ci;
			fy=y-ay-boxy*cj;
			fz=z-az-boxz*ck;
		}
		inline int region_index(int ci,int cj,int ck,int ei,int ej,int ek,double &qx,double &qy,double &qz,int disp) {
			if(xperiodic) {if(ci+ei<nx) {ei+=nx;qx=-(bx-ax);} else if(ci+ei>=(nx<<1)) {ei-=nx;qx=bx-ax;} else qx=0;}
			if(yperiodic) {if(cj+ej<ny) {ej+=ny;qy=-(by-ay);} else if(cj+ej>=(ny<<1)) {ej-=ny;qy=by-ay;} else qy=0;}
			if(zperiodic) {if(ck+ek<nz) {ek+=nz;qz=-(bz-az);} else if(ck+ek>=(nz<<1)) {ek-=nz;qz=bz-az;} else qz=0;}
			return disp+ei+nx*(ej+ny*ek);
		}
		void create_all_images();
		void check_compartmentalized();
	protected:
		void add_particle_memory(int i);
		inline bool put_locate_block(int &ijk,double &x,double &y,double &z);
		inline bool put_remap(int &ijk,double &x,double &y,double &z);
};

class container_periodic : public container_periodic_base {
	public:
		container_periodic(double bx_,double bxy_,double by_,double bxz_,double byz_,double bz_,
				int nx_,int ny_,int nz_,bool xperiodic_,int init_mem);
		void clear();
		void put(int n,double x,double y,double z);
		void put(voropp_order &vo,int n,double x,double y,double z);
		void import(FILE *fp=stdin);
		void import(voropp_order &vo,FILE *fp=stdin);
		/** Imports a list of particles from an open file stream into
		 * the container. Entries of four numbers (Particle ID, x
		 * position, y position, z position) are searched for. If the
		 * file cannot be successfully read, then the routine causes a
		 * fatal error.
		 * \param[in] filename the name of the file to open and read
		 *                     from. */
		inline void import(const char* filename) {
			FILE *fp(voropp_safe_fopen(filename,"r"));
			import(fp);
			fclose(fp);
		}
		/** Imports a list of particles from an open file stream into
		 * the container. Entries of four numbers (Particle ID, x
		 * position, y position, z position) are searched for. In
		 * addition, the order in which particles are read is saved
		 * into an ordering class. If the file cannot be successfully
		 * read, then the routine causes a fatal error.
		 * \param[in,out] vo the ordering class to use.
		 * \param[in] filename the name of the file to open and read
		 *                     from. */
		inline void import(voropp_order &vo,const char* filename) {
			FILE *fp(voropp_safe_fopen(filename,"r"));
			import(vo,fp);
			fclose(fp);
		}
		void compute_all_cells();
		double sum_cell_volumes();
		/** Dumps particle IDs and positions to a file.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_particles(v_loop &vl,FILE *fp) {
			double *pp;
			if(vl.start()) do {
				pp=p[vl.ijk]+3*vl.q;
				fprintf(fp,"%d %g %g %g\n",id[vl.ijk][vl.q],*pp,pp[1],pp[2]);
			} while(vl.inc());
		}
		/** Dumps all of the particle IDs and positions to a file.
		 * \param[in] fp a file handle to write to. */
		inline void draw_particles(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_particles(vl,fp);
		}
		/** Dumps all of the particle IDs and positions to a file.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_particles(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_particles(fp);
			fclose(fp);
		}
		/** Dumps particle positions in POV-Ray format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_particles_pov(v_loop &vl,FILE *fp) {
			double *pp;
			if(vl.start()) do {
				pp=p[vl.ijk]+3*vl.q;
				fprintf(fp,"// id %d\nsphere{<%g,%g,%g>,s}\n",
						id[vl.ijk][vl.q],*pp,pp[1],pp[2]);
			} while(vl.inc());
		}
		/** Dumps all particle positions in POV-Ray format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_particles_pov(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_particles_pov(vl,fp);
		}
		/** Dumps all particle positions in POV-Ray format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_particles_pov(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_particles_pov(fp);
			fclose(fp);
		}
		/** Computes Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_cells_gnuplot(v_loop &vl,FILE *fp) {
			voronoicell c;double *pp;
			if(vl.start()) do if(compute_cell(c,vl)) {
				pp=p[vl.ijk]+ps*vl.q;
				c.draw_gnuplot(*pp,pp[1],pp[2],fp);
			} while(vl.inc());
		}
		/** Computes all Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_cells_gnuplot(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_cells_gnuplot(vl,fp);
		}
		/** Compute all Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_cells_gnuplot(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_cells_gnuplot(fp);
			fclose(fp);
		}
		/** Computes Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_cells_pov(v_loop &vl,FILE *fp) {
			voronoicell c;double *pp;
			if(vl.start()) do if(compute_cell(c,vl)) {
				fprintf(fp,"// cell %d\n",id[vl.ijk][vl.q]);
				pp=p[vl.ijk]+ps*vl.q;
				c.draw_pov(*pp,pp[1],pp[2],fp);
			} while(vl.inc());
		}
		/** Computes all Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_cells_pov(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_cells_pov(vl,fp);
		}
		/** Computes all Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_cells_pov(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_cells_pov(fp);
			fclose(fp);
		}
		/** Computes the Voronoi cells and saves customized information
		 * about them.
		 * \param[in] vl the loop class to use.
		 * \param[in] format the custom output string to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void print_custom(v_loop &vl,const char *format,FILE *fp) {
			int ijk,q;double *pp;
			if(contains_neighbor(format)) {
				voronoicell_neighbor c;
				if(vl.start()) do if(compute_cell(c,vl)) {
					ijk=vl.ijk;q=vl.q;pp=p[ijk]+ps*q;
					c.output_custom(format,id[ijk][q],*pp,pp[1],pp[2],default_radius,fp);
				} while(vl.inc());
			} else {
				voronoicell c;
				if(vl.start()) do if(compute_cell(c,vl)) {
					ijk=vl.ijk;q=vl.q;pp=p[ijk]+ps*q;
					c.output_custom(format,id[ijk][q],*pp,pp[1],pp[2],default_radius,fp);
				} while(vl.inc());
			}
		}
		void print_custom(const char *format,FILE *fp=stdout);
		void print_custom(const char *format,const char *filename);
		template<class v_cell,class v_loop>
		inline bool compute_cell(v_cell &c,v_loop &vl) {
			return vc.compute_cell(c,vl.ijk,vl.q,vl.i,vl.j,vl.k);
		}
		template<class v_cell>
		inline bool compute_cell(v_cell &c,int ijk,int q) {
			int k(ijk/nxy),ijkt(ijk-nxy*k),j(ijkt/nx),i(ijkt-j*nx);
			return vc.compute_cell(c,ijk,q,i,j,k);
		}		
	private:
		voropp_compute<container> vc;
		inline void r_init(int ijk,int s) {};
		inline double r_cutoff(double lrs) {return lrs;}
		inline double r_scale(double rs,int ijk,int q) {return rs;}
		friend class voropp_compute<container>;
};

class container_periodic_poly : public container_periodic_base {
	public:
		/** The current maximum radius of any particle, used to
		 * determine when to cut off the radical Voronoi computation.
		 * */
		double max_radius;
		container_periodic_poly(double bx_,double bxy_,double by_,double bxz_,double byz_,double bz_,
				int nx_,int ny_,int nz_,bool xperiodic_,int init_mem);
		void clear();
		void put(int n,double x,double y,double z,double r);
		void put(voropp_order &vo,int n,double x,double y,double z,double r);
		void import(FILE *fp=stdin);
		void import(voropp_order &vo,FILE *fp=stdin);
		/** Imports a list of particles from an open file stream into
		 * the container_poly class. Entries of five numbers (Particle
		 * ID, x position, y position, z position, radius) are searched
		 * for. If the file cannot be successfully read, then the
		 * routine causes a fatal error.
		 * \param[in] filename the name of the file to open and read
		 *                     from. */
		inline void import(const char* filename) {
			FILE *fp(voropp_safe_fopen(filename,"r"));
			import(fp);
			fclose(fp);
		}
		/** Imports a list of particles from an open file stream into
		 * the container_poly class. Entries of five numbers (Particle
		 * ID, x position, y position, z position, radius) are searched
		 * for. In addition, the order in which particles are read is
		 * saved into an ordering class. If the file cannot be
		 * successfully read, then the routine causes a fatal error.
		 * \param[in,out] vo the ordering class to use.
		 * \param[in] filename the name of the file to open and read
		 *                     from. */
		inline void import(voropp_order &vo,const char* filename) {
			FILE *fp(voropp_safe_fopen(filename,"r"));
			import(vo,fp);
			fclose(fp);
		}
		void compute_all_cells();
		double sum_cell_volumes();
		/** Dumps particle IDs, positions and radii to a file.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_particles(v_loop &vl,FILE *fp) {
			double *pp;
			if(vl.start()) do {
				pp=p[vl.ijk]+4*vl.q;
				fprintf(fp,"%d %g %g %g %g\n",id[vl.ijk][vl.q],*pp,pp[1],pp[2],pp[3]);
			} while(vl.inc());
		}
		/** Dumps all of the particle IDs, positions and radii to a
		 * file.
		 * \param[in] fp a file handle to write to. */
		inline void draw_particles(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_particles(vl,fp);
		}
		/** Dumps all of the particle IDs, positions and radii to a
		 * file.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_particles(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_particles(fp);
			fclose(fp);
		}
		/** Dumps particle positions in POV-Ray format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_particles_pov(v_loop &vl,FILE *fp) {
			double *pp;
			if(vl.start()) do {
				pp=p[vl.ijk]+4*vl.q;
				fprintf(fp,"// id %d\nsphere{<%g,%g,%g>,%g}\n",
						id[vl.ijk][vl.q],*pp,pp[1],pp[2],pp[3]);
			} while(vl.inc());
		}
		/** Dumps all the particle positions in POV-Ray format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_particles_pov(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_particles_pov(vl,fp);
		}
		/** Dumps all the particle positions in POV-Ray format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_particles_pov(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_particles_pov(fp);
			fclose(fp);
		}
		/** Computes Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_cells_gnuplot(v_loop &vl,FILE *fp) {
			voronoicell c;double *pp;
			if(vl.start()) do if(compute_cell(c,vl)) {
				pp=p[vl.ijk]+ps*vl.q;
				c.draw_gnuplot(*pp,pp[1],pp[2],fp);
			} while(vl.inc());
		}
		/** Compute all Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_cells_gnuplot(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_cells_gnuplot(vl,fp);
		}
		/** Compute all Voronoi cells and saves the output in gnuplot
		 * format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_cells_gnuplot(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_cells_gnuplot(fp);
			fclose(fp);
		}
		/** Computes Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] vl the loop class to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void draw_cells_pov(v_loop &vl,FILE *fp) {
			voronoicell c;double *pp;
			if(vl.start()) do if(compute_cell(c,vl)) {
				fprintf(fp,"// cell %d\n",id[vl.ijk][vl.q]);
				pp=p[vl.ijk]+ps*vl.q;
				c.draw_pov(*pp,pp[1],pp[2],fp);
			} while(vl.inc());
		}
		/** Computes all Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] fp a file handle to write to. */
		inline void draw_cells_pov(FILE *fp=stdout) {
			v_loop_all vl(*this);
			draw_cells_pov(vl,fp);
		}
		/** Computes all Voronoi cells and saves the output in POV-Ray
		 * format.
		 * \param[in] filename the name of the file to write to. */
		inline void draw_cells_pov(const char *filename) {
			FILE *fp(voropp_safe_fopen(filename,"w"));
			draw_cells_pov(fp);
			fclose(fp);
		}
		/** Computes the Voronoi cells and saves customized information
		 * about them.
		 * \param[in] vl the loop class to use.
		 * \param[in] format the custom output string to use.
		 * \param[in] fp a file handle to write to. */
		template<class v_loop>
		void print_custom(v_loop &vl,const char *format,FILE *fp) {
			int ijk,q;double *pp;
			if(contains_neighbor(format)) {
				voronoicell_neighbor c;
				if(vl.start()) do if(compute_cell(c,vl)) {
					ijk=vl.ijk;q=vl.q;pp=p[ijk]+ps*q;
					c.output_custom(format,id[ijk][q],*pp,pp[1],pp[2],pp[3],fp);
				} while(vl.inc());
			} else {
				voronoicell c;
				if(vl.start()) do if(compute_cell(c,vl)) {
					ijk=vl.ijk;q=vl.q;pp=p[ijk]+ps*q;
					c.output_custom(format,id[ijk][q],*pp,pp[1],pp[2],pp[3],fp);
				} while(vl.inc());
			}
		}
		template<class v_cell,class v_loop>
		inline bool compute_cell(v_cell &c,v_loop &vl) {
			return vc.compute_cell(c,vl.ijk,vl.q,vl.i,vl.j,vl.k);
		}
		template<class v_cell>
		inline bool compute_cell(v_cell &c,int ijk,int q) {
			int k(ijk/nxy),ijkt(ijk-nxy*k),j(ijkt/nx),i(ijkt-j*nx);
			return vc.compute_cell(c,ijk,q,i,j,k);
		}		
		void print_custom(const char *format,FILE *fp=stdout);
		void print_custom(const char *format,const char *filename);
	private:
		voropp_compute<container_poly> vc;
		double r_rad,r_mul;
		inline void r_init(int ijk,int s) {
			r_rad=p[ijk][4*s+3];
			r_mul=1+(r_rad*r_rad-max_radius*max_radius)/((max_radius+r_rad)*(max_radius+r_rad));
			r_rad*=r_rad;
		}
		inline double r_cutoff(double lrs) {
			return r_mul*lrs;
		}
		inline double r_scale(double rs,int ijk,int q) {
			return rs+r_rad-p[ijk][4*q+3]*p[ijk][4*q+3];
		}
		friend class voropp_compute<container_poly>;
};

#endif