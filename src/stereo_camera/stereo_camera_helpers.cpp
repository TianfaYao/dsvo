#include "stereo_camera/stereo_camera.hpp"

void StereoCamera::updateConfig(direct_stereo::DirectStereoConfig &config, uint32_t level) {
	// std::cout<<"Init param"<<std::endl;
	MAX_REPROJ_DIST = config.MAX_REPROJ_DIST;
	PROP_POSE_PYMD = config.PROP_POSE_PYMD;
	PROP_POSE_ITER = config.PROP_POSE_ITER;
	KP_BLOCKS = config.KP_BLOCKS;
	BLUR_SZ = 2*config.BLUR_SZ+1;
	BLUR_VAR = config.BLUR_VAR;
	// INIT_MAX_KP = config.INIT_MAX_KP;
	// QUAL_LEVEL = config.QUAL_LEVEL;
	// MIN_FEATURE_DIST = config.MIN_FEATURE_DIST;
	BA_MAX_STEP = config.BA_MAX_STEP;
	SCALE_MAX_STEP = config.SCALE_MAX_STEP;
	SCALE_PYMD = config.SCALE_PYMD;
	MONO_INLIER_THRES = config.MONO_INLIER_THRES;
	LOOP_CLOSURE = config.LOOP_CLOSURE;
	MIN_TRACK_DIST = config.MIN_TRACK_DIST;
	MIN_TRACK_POINTS = config.MIN_TRACK_POINTS;
	FAST_THRES = config.FAST_THRES;
	MAX_FEATURES = config.MAX_FEATURES;
	REFINE_PIXEL = config.REFINE_PIXEL;
	DEBUG_FEATURE = config.DEBUG_FEATURE;
	FEATURE_OF_PYMD = config.FEATURE_OF_PYMD;
	INIT_SCALE = config.INIT_SCALE;
	TEST_STEREO = config.TEST_STEREO;
	cv::destroyAllWindows();

	if(time_ofs.is_open()) {
		time_ofs.close();
	}
	time_ofs.open("time.txt", std::ofstream::out | std::ofstream::trunc);

    param_changed = true;
}

StereoCamera::StereoCamera(const std::vector<double>& E0,
						   const std::vector<double>& K0,
						   const std::vector<double>& frame_size0,
						   const std::vector<double>& dist_coeff0,
						   const std::vector<double>& E1,
						   const std::vector<double>& K1,
						   const std::vector<double>& frame_size1,
						   const std::vector<double>& dist_coeff1,
						   bool cvt2VGA, const string& gt_type) {
	cv::Mat cam0_E = cv::Mat(E0);
	cam0_E = cam0_E.reshape(0,4);
	cv::Mat R0(cam0_E, cv::Rect(0,0,3,3));
	cv::Mat t0 = cam0_E(cv::Rect(3,0,1,3));

	camera0.K = cv::Mat::zeros(3,3,CV_64F);
	camera0.K.at<double>(0,0) = K0[0];
	camera0.K.at<double>(1,1) = K0[1];
	camera0.K.at<double>(0,2) = K0[2];
	camera0.K.at<double>(1,2) = K0[3];
	camera0.K.at<double>(2,2) = 1.0;

	cv::Size cam0_frame_size = cv::Size(frame_size0[0], frame_size0[1]);

	cv::Mat cam1_E = cv::Mat(E1);
	cam1_E = cam1_E.reshape(0,4);
	cv::Mat R1 = cam1_E(cv::Rect(0,0,3,3));
	cv::Mat t1 = cam1_E(cv::Rect(3,0,1,3));

	camera1.K = cv::Mat::zeros(3,3,CV_64F);
	camera1.K.at<double>(0,0) = K1[0];
	camera1.K.at<double>(1,1) = K1[1];
	camera1.K.at<double>(0,2) = K1[2];
	camera1.K.at<double>(1,2) = K1[3];
	camera1.K.at<double>(2,2) = 1.0;

	cv::Size cam1_frame_size = cv::Size(frame_size1[0], frame_size1[1]);


	cv::Mat R1T;
	cv::transpose(R1, R1T);
	StereoModel stereo0;
	stereo0.R = R1T * R0;
	stereo0.t = R1T * (t0 - t1);

	assert(cam0_frame_size = cam1_frame_size);

	frame_size = cam0_frame_size;
  if(cvt2VGA) {
  	frame_size = cv::Size(640,480);
  }

  cv::Mat rect_R0, rect_R1, rect_P0, rect_P1;
  cv::stereoRectify(camera0.K, dist_coeff0, camera1.K, dist_coeff1,cam0_frame_size, stereo0.R, stereo0.t,
  	rect_R0, rect_R1, rect_P0, rect_P1, cam0_Q, cv::CALIB_ZERO_DISPARITY, 0, frame_size);

  cv::initUndistortRectifyMap(camera0.K, dist_coeff0, rect_R0, rect_P0, frame_size,CV_32F, camera0.rect_map_x, camera0.rect_map_y);
  cv::initUndistortRectifyMap(camera1.K, dist_coeff1, rect_R1, rect_P1, frame_size, CV_32F, camera1.rect_map_x, camera1.rect_map_y);

    // modify external matrix by rect rotation
	cv::Mat rR0T, rR1T;
	cv::transpose(rect_R0, rR0T);
	cv::transpose(rect_R1, rR1T);
  R0 = R0 * rR0T;
  R1 = R1 * rR1T;

	for(int i=0; i<3; i++) {
		for(int j=0; j<3; j++) {
			camera0.R_C2B(i,j) = R0.at<double>(i,j);
			camera1.R_C2B(i,j) = R1.at<double>(i,j);
		}
		camera0.t_C2B(i) = t0.at<double>(i,0);
		camera1.t_C2B(i) = t1.at<double>(i,0);
	}
	camera0.R_B2C = camera0.R_C2B.transpose();
	camera0.t_B2C = - camera0.R_B2C * camera0.t_C2B;
	camera1.R_B2C = camera1.R_C2B.transpose();
	camera1.t_B2C = - camera1.R_B2C * camera1.t_C2B;

  cv::Mat tmp = rect_P0(cv::Rect(0,0,3,3));
  tmp.copyTo(camera0.K);
  tmp = rect_P1(cv::Rect(0,0,3,3));
  tmp.copyTo(camera1.K);

	stereo0.R = cv::Mat::eye(3,3,CV_64F);
	tmp = rect_P1(cv::Rect(3,0,1,3));
	tmp.copyTo(stereo0.t);
	stereo0.t = stereo0.t / camera0.K.at<double>(0,0);

	StereoModel stereo1;
	stereo1.R = stereo0.R;
	stereo1.t = -stereo0.t;

	camera1.stereo = stereo0;
	camera0.stereo = stereo1;

	// std::cout<<stereo0.t<<std::endl;
	direct_pcl_pub = nh.advertise<PointCloud> ("direct_points", 1);
	stereo_pcl_pub = nh.advertise<PointCloud> ("stereo_points", 1);
    pose_pub = nh.advertise<geometry_msgs::PoseStamped>("cam_pose", 1000);

	// comparer type
	if(gt_type == "odom")
	{
		comparer = new OdomComparer();
	} else if(gt_type == "point") {
		comparer = new PointComparer();
	} else {
		comparer = new TransComparer();
	}

	f = boost::bind(&StereoCamera::updateConfig, this, _1, _2);
  	server.setCallback(f);

    frame_dropped_count = 0;
    // directSolver_ptr = shared_ptr<DirectSolver>(new DirectSolver());
    param_changed = true;
}

void StereoCamera::detectFeatures(const cv::Mat& img, std::vector<cv::KeyPoint>& kps) {
	cv::Ptr<cv::FastFeatureDetector> detector = cv::FastFeatureDetector::create(FAST_THRES);
	// std::vector<cv::KeyPoint> kp;
	// detector->detect(img, kp, cv::Mat());
 //  cv::KeyPointsFilter::removeDuplicated( kp );
	// cv::KeyPointsFilter::retainBest(kp, MAX_FEATURES);
 //  for(int k=0; k<kp.size(); k++) {
 //  	kps.push_back(kp[k]);
 //  }

	int subCols = img.cols / KP_BLOCKS;
	int subRows = img.rows / KP_BLOCKS;
  for(int i=0; i<KP_BLOCKS; i++) {
  	for(int j=0; j<KP_BLOCKS; j++) {
  		cv::Rect subRegion = cv::Rect(i*subCols, j*subRows, subCols, subRows);
			std::vector<cv::KeyPoint> kp;
			detector->detect(img(subRegion),kp, cv::Mat());
		  cv::KeyPointsFilter::removeDuplicated( kp );
			cv::KeyPointsFilter::retainBest(kp, ceil(ceil(MAX_FEATURES/KP_BLOCKS)/KP_BLOCKS));
  		// std::vector<cv::Point2f> subFeatures0;
		  // cv::goodFeaturesToTrack(img(subRegion), subFeatures0, INIT_MAX_KP, QUAL_LEVEL, MIN_FEATURE_DIST);
		  for(int k=0; k<kp.size(); k++) {
		  	kp[k].pt.x = kp[k].pt.x+i*subCols;
		  	kp[k].pt.y = kp[k].pt.y+j*subRows;
		  	kps.push_back(kp[k]);
		  }
  	}
  }
}

void StereoCamera::triangulateByStereoMatch(KeyFrame& keyframe, const CameraModel& cam) {
	float MAX_DISP = 50.0;
	std::clock_t triangulateByStereoMatch_start = std::clock();
	// find stereo match by row matching
	cv::Ptr<cv::FastFeatureDetector> detector = cv::FastFeatureDetector::create(FAST_THRES);
	std::vector<cv::KeyPoint> kp0, kp1;
	detectFeatures(keyframe.img0, kp0);
	detectFeatures(keyframe.img1, kp1);
	if(kp0.size() < MIN_TRACK_POINTS || kp1.size() < MIN_TRACK_POINTS) return;

  std::sort(kp0.begin(), kp0.end(), KeyPoint12_LessThan_y());
  std::sort(kp1.begin(), kp1.end(), KeyPoint12_LessThan_y());

	cv::Ptr<cv::ORB> descriptor = cv::ORB::create();
	cv::Mat desc0, desc1;
	descriptor->compute(keyframe.img0, kp0, desc0);
	descriptor->compute(keyframe.img1, kp1, desc1);

	PointCloud::Ptr point_cloud (new PointCloud);
	std::vector<cv::Point2f> kp0_in, kp1_in;
  std::vector<cv::DMatch> matches;
	cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create ("BruteForce");
  int i(0), j(0), jj(0);
  while(i<kp0.size() && kp1[j].pt.y-kp0[i].pt.y > 1.0) i++;
  for(; i<kp0.size(); i++)
  {
  cv::Mat mask(cv::Mat::zeros(1, desc1.rows, CV_8UC1));
  bool worthMatch = false;

  while(j<kp1.size() && kp0[i].pt.y-kp1[j].pt.y > 1.0) j++;

  jj = j;
  while(jj<kp1.size() && fabs(kp1[jj].pt.y - kp0[i].pt.y) <= 1.0) {
  	mask.at<uchar>(jj++) = true;
  	worthMatch = true;
  }

  if(!worthMatch) continue;

  const cv::Mat& query_desc0 = desc0.row(i);
  std::vector<cv::DMatch> match_candidates;
  matcher->match(query_desc0, desc1, match_candidates, mask);

  if (match_candidates.empty()) continue;
  float d = kp0[i].pt.x - kp1[match_candidates[0].trainIdx].pt.x;
  if (!(d > 0 && d<MAX_DISP)) continue;

  match_candidates[0].queryIdx = i;
  matches.push_back(match_candidates[0]);

	kp0_in.push_back(kp0[i].pt);
	kp1_in.push_back(kp1[match_candidates[0].trainIdx].pt);
  }

  std::vector<cv::Point2f> kp1_in_ori = kp1_in;
  cv::Mat match_img;
  cv::drawMatches(keyframe.img0, kp0, keyframe.img1, kp1, matches, match_img);
  cv::imshow("Stereo match", match_img);
  cv::waitKey(1);

	reconstructor.refinePixel(keyframe.img0, keyframe.img1, kp0_in, kp1_in);

  float f = cam.K.at<double>(0,0);
  float cx = cam.K.at<double>(0,2);
  float cy = cam.K.at<double>(1,2);
  float B = cam.stereo.t.at<double>(0,0);
  float fB = f*B;
  for(int i=0; i<kp0_in.size(); i++) {
  float d = kp0_in[i].x - kp1_in[i].x;
  if (!(d > 0 && d<MAX_DISP)) {
  	std::cout<<"d= "<<d<<std::endl;
  	continue;
  }
// std::cout<<kp1_in_ori[i].x<<" - "<<kp1_in[i].x<<std::endl;
  float z = fB / d;
  float x = (kp0_in[i].x - cx) * z / f;
  float y = (kp0_in[i].y - cy) * z / f;
  keyframe.feature_points.features.push_back(kp0_in[i]);
  keyframe.feature_points.points.push_back(cv::Point3f(x,y,z));

  pcl::PointXYZ p;
  p.x = x;
  p.y = y;
  p.z = z;
  point_cloud->push_back(p);
  }

  point_cloud->header.frame_id = "/map";
  stereo_pcl_pub.publish(point_cloud);

	// std::cout << "triangulateByStereoMatch: " << (std::clock() - triangulateByStereoMatch_start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
}

KeyFrame StereoCamera::createKeyFrame(const Pose& cur_pose, const cv::Mat& cur_img0, const cv::Mat& cur_img1, const FeaturePoints& feature_points, const CameraModel& cam0){
	KeyFrame keyframe = KeyFrame();

	keyframe.pose = cur_pose;
	keyframe.time = cur_time;
	keyframe.img0 = cur_img0.clone();
	keyframe.img1 = cur_img1.clone();

	if(feature_points.features.empty()) {
		triangulateByStereoMatch(keyframe, cam0);		// use stereo match to triangulate points if necessary
	} else {
	  keyframe.feature_points = feature_points;
	}

	// publish point cloud
	PointCloud::Ptr point_cloud (new PointCloud);
	for(int i=0; i<keyframe.feature_points.points.size(); i++) {
		// std::cout<<lastKF_pts[i]<<std::endl;
		pcl::PointXYZ p;
		p.x = keyframe.feature_points.points[i].y;
		p.y = keyframe.feature_points.points[i].z;
		p.z = keyframe.feature_points.points[i].x;
		point_cloud->push_back(p);
	}
	point_cloud->header.frame_id = "/map";
	direct_pcl_pub.publish(point_cloud);

  // detect feature points
  std::vector<cv::KeyPoint> kp0;
	detectFeatures(keyframe.img0, kp0);
	for(int i=0; i<kp0.size(); i++) {
		keyframe.features0.push_back(kp0[i].pt);
	}

  keyframe.init_feature0_count = keyframe.features0.size();
  // // put previous features into new KF for consistency
  // for(int i=0; i<feature_points.features.size(); i++) {
  // 	keyframe.features0.push_back(feature_points.features[i]);
  // }

  if(DEBUG_FEATURE) {
	  cv::Mat feature_img = keyframe.img0.clone();
	  cv::cvtColor(feature_img, feature_img, cv::COLOR_GRAY2BGR);
	  for(int i=0; i<keyframe.features0.size(); i++) {
	  	cv::circle(feature_img, keyframe.features0[i], 5, cv::Scalar(0,255,0));
	  }
	  std::cout<<"KF init features "<<keyframe.features0.size()<<std::endl;
	  cv::imshow("KF init features", feature_img);
	  cv::waitKey(1);
	}

  return keyframe;
}
